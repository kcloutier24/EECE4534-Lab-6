/* LAB4 AUDIO WITH FIFO */

#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/list.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <asm/spinlock.h>
#include <linux/uaccess.h>
#include <linux/kfifo.h>
#include <linux/delay.h>
#include <linux/irqflags.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <asm/io.h>

#define DRIVER_NAME "esl-audio"

#define ISR_OFFSET 0x0 //Interrupt Status Register
#define IER_OFFSET 0x4 //Interrupt Enable Register
#define TXFIFO_DATA_OFFSET 0x10
#define TDFV_OFFSET 0xC //Transmit Data FIFO Vacancy (TDFV)
#define TLR_OFFSET 0x14
#define RDFO_OFFSET 0x1C
#define TFPF_MASK (0x1 << 22) //Transmit FIFO Programmable Full 1 means interupt pending 0 means no interupt pending 
#define TFPE_MASK (0x1 << 21) //Transmit FIFO Programmable Empty
#define TPOE_MASK (0x1 << 25)
#define TSE_MASK (0x1 << 28)
#define TC_MASK (0x1 << 26)
#define RC_MASK (0x1 << 27)

#define INTERRUPT_MASK  (TFPE_MASK | TPOE_MASK | TSE_MASK)
#define RFPF_MASK (0x1 << 20) //Receive FIFO Programmable Full
#define TDFR_OFFSET 0x8 //Transmit Data FIFO Reset (TDFR) 
#define RDFR_OFFSET 0x18
#define TDR_OFFSET 0x2C //Transmit Destination Register (TDR)

#define ISR_CLEAR_MASK 0xFFFFFFFF
/* fifo size in elements (bytes) */
#define FIFO_SIZE	32

// forward declaration of axi_i2s
struct axi_i2s;

static int irqnum; //AXI FIFO IRQ Number


// declare functions we will use from the esli2s driver
extern void esl_codec_enable_tx(struct axi_i2s* i2s);
extern void esl_codec_disable_tx(struct axi_i2s* i2s);

// structure for an individual instance (ie. one audio driver)
struct esl_audio_instance
{
  void* __iomem regs; //fifo registers
  struct cdev chr_dev; //character device
  dev_t devno;

  // list head
  struct list_head inst_list;

  // interrupt number
  unsigned int irqnum;

  // fifo depth
  unsigned int tx_fifo_depth;

  // wait queue
  wait_queue_head_t waitq;

  // i2s controller instance
  struct axi_i2s* i2s_controller;
};

// structure for class of all audio drivers
struct esl_audio_driver
{
  dev_t first_devno;
  struct class* class;
  unsigned int instance_count;     // how many drivers have been instantiated?
  struct list_head instance_list;  // pointer to first instance
};

// allocate and initialize global data (class level)
static struct esl_audio_driver driver_data = {
  .instance_count = 0,
  .instance_list = LIST_HEAD_INIT(driver_data.instance_list),
};

/* Utility Functions */

// Utility method to read registers
static inline u32 reg_read(struct esl_audio_instance *inst, u32 reg)
{
  return ioread32(inst->regs + reg);
}

// Utility method to write registers
static inline void reg_write(struct esl_audio_instance *inst, u32 reg,
                             u32 value)
{
  iowrite32(value, inst->regs + reg);
}

// find instance from inode using minor number and linked list
static struct esl_audio_instance* inode_to_instance(struct inode* i)
{
  struct esl_audio_instance *inst_iter;
  unsigned int minor = iminor(i);

  // start with fist element in linked list (stored at class),
  // and iterate through its elements
  list_for_each_entry(inst_iter, &driver_data.instance_list, inst_list)
    {
      // found matching minor number?
      if (MINOR(inst_iter->devno) == minor)
        {
          // return instance pointer of corresponding instance
          return inst_iter;
        }
    }

  // not found
  return NULL;
}

void fifo_initialize(struct esl_audio_instance *inst)
{
  reg_write(inst, TDFR_OFFSET, 0xA5);
  reg_write(inst, RDFR_OFFSET, 0xA5);

  reg_write(inst, ISR_OFFSET, ISR_CLEAR_MASK);

  reg_write(inst, IER_OFFSET, INTERRUPT_MASK);
  reg_write(inst, TDR_OFFSET, 0x2);
}

int fifo_full(struct esl_audio_instance *inst)
{
  return (reg_read(inst, TDFV_OFFSET) == 0x0);
}

void fifo_transmit_word(struct esl_audio_instance *inst, u32 word)
{
  //printk(KERN_INFO "Writing word: 0x%X", word);
  reg_write(inst, TXFIFO_DATA_OFFSET, word);
  reg_write(inst, TLR_OFFSET, 0x4);
  //printk(KERN_INFO "Done transmitting, before return\n");
}



// return instance struct based on file
static struct esl_audio_instance* file_to_instance(struct file* f)
{
  return inode_to_instance(f->f_path.dentry->d_inode);
}

/* Character device File Ops */
static ssize_t esl_audio_write(struct file* f,
                               const char __user *buf, size_t len,
                               loff_t* offset)
{
    struct esl_audio_instance *inst = file_to_instance(f);

    static char kernbuf[32];
    u32 word = 0x0;

    copy_from_user(kernbuf, buf, (len > 32 ? 32 : len));

    if (!inst)
    {
        // instance not found
        return -ENOENT;
    }

    //write to AXI FIFO
    word = ((u32)(kernbuf[3]) << 24) + ((u32)(kernbuf[2]) << 16) + ((u32)(kernbuf[1]) << 8) + ((u32)(kernbuf[0]) << 0);


    //while(fifo_full(inst))
    //{
    //    usleep_range(6000,8300);
    //}

    wait_event_interruptible(inst->waitq, !fifo_full(inst));

    fifo_transmit_word(inst, word);

    return 4;
}

// definition of file operations 
struct file_operations esl_audio_fops = {
  .write = esl_audio_write,
};



/* interrupt handler */
static irqreturn_t esl_audio_irq_handler(int irq, void* dev_id)
{
  struct esl_audio_instance* inst = dev_id;
  irqnum = reg_read(inst, ISR_OFFSET);

  printk(KERN_INFO "zedaudio: Interrupt Triggered\n");

  // Size

  // Overrun

  if ((irqnum & TFPF_MASK) > 0)
  {
    //printk(KERN_INFO "zedaudio: Transmit FIFO Programmable Full\n");
    
    reg_write(inst, ISR_OFFSET, TFPF_MASK);
  }
  else if((irqnum & TPOE_MASK) > 0)
  {
    reg_write(inst, ISR_OFFSET, TPOE_MASK);
  }
  else if((irqnum & TSE_MASK) > 0)
  {
    reg_write(inst, ISR_OFFSET, TSE_MASK);
  }
  else if((irqnum & TFPE_MASK) > 0)
  {
    printk(KERN_INFO "zedaudio: Transmit FIFO Programmable Empty\n");
    reg_write(inst, ISR_OFFSET, TFPE_MASK);
    wake_up(&inst->waitq);
  }
  return IRQ_HANDLED;
}

static int esl_audio_probe(struct platform_device* pdev)
{
  struct esl_audio_instance* inst = NULL;
  int err;
  struct resource* res;
  struct device *dev;
  const void* prop;
  phandle i2s_phandle;
  struct device_node* i2sctl_node;

  printk(KERN_INFO "zedaudio: Started probing...\n");

  // allocate instance
  inst = devm_kzalloc(&pdev->dev, sizeof(struct esl_audio_instance),
                      GFP_KERNEL);

  // set platform driver data
  platform_set_drvdata(pdev, inst);

  // get registers (AXI FIFO)
  res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
  if (IS_ERR(res))
    {
      return PTR_ERR(res);
    }

  inst->regs = devm_ioremap_resource(&pdev->dev, res);
  if (IS_ERR(inst->regs))
    {
      return PTR_ERR(inst->regs);
    }

  // get TX fifo depth
  err = of_property_read_u32(pdev->dev.of_node, "xlnx,tx-fifo-depth",
                             &inst->tx_fifo_depth);
  if (err)
    {
      printk(KERN_ERR "%s: failed to retrieve TX fifo depth\n",
             DRIVER_NAME);
      return err;
    }

  // get interrupt
  res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
  if (IS_ERR(res))
    {
      return PTR_ERR(res);
    }

  err = devm_request_irq(&pdev->dev, res->start,
                         esl_audio_irq_handler,
                         IRQF_TRIGGER_HIGH,
                         "zedaudio", inst);
  if (err < 0)
    {
      return err;
    }

  // save irq number
  inst->irqnum = res->start;

  // get and inspect i2s reference
  prop = of_get_property(pdev->dev.of_node, "esl,i2s-controller", NULL);
  if (IS_ERR(prop))
    {
      return PTR_ERR(prop);
    }

  // cast into phandle
  i2s_phandle = be32_to_cpu(*(phandle*)prop);
  i2sctl_node = of_find_node_by_phandle(i2s_phandle);
  if (!i2sctl_node)
    {
      // couldnt find
      printk(KERN_ERR "%s: could not find AXI I2S controller", DRIVER_NAME);
      return -ENODEV;
    }

  // get controller instance pointer from device node
  inst->i2s_controller = i2sctl_node->data;
  of_node_put(i2sctl_node);

  // create character device
  // get device number
  inst->devno = MKDEV(MAJOR(driver_data.first_devno),
                      driver_data.instance_count);

  // TODO initialize and create character device
  inst->devno = MKDEV(MAJOR(driver_data.first_devno),
                      driver_data.instance_count);

  
  cdev_init(&inst->chr_dev, &esl_audio_fops);

  err = cdev_add(&inst->chr_dev, inst->devno, 1);
  if(err)
  {
    unregister_chrdev_region(inst->devno, 1);
    return err;
  }

  dev = device_create(driver_data.class, NULL, inst->devno, NULL, "zedaudio%d", driver_data.instance_count);
  if(IS_ERR(dev))
  {
    printk(KERN_ERR "%s: could not create chrdev", DRIVER_NAME);
    return PTR_ERR(dev);
  }

  // increment instance count
  driver_data.instance_count++;

  // put into list
  INIT_LIST_HEAD(&inst->inst_list);
  list_add(&inst->inst_list, &driver_data.instance_list);

  // init wait queue
  init_waitqueue_head(&inst->waitq);

  //reset AXI FIFO
  fifo_initialize(inst);

  //enable interrupts
  esl_audio_irq_handler(irqnum, dev);

  printk(KERN_INFO "zedaudio: Done probing...\n");

  return 0;
}

static int esl_audio_remove(struct platform_device* pdev)
{
  struct esl_audio_instance* inst = platform_get_drvdata(pdev);

  fifo_initialize(inst);

  // TODO remove all traces of character device
  cdev_del(&inst->chr_dev);
  device_destroy(driver_data.class, inst->devno);

  // remove from list
  list_del(&inst->inst_list);

  return 0;
}

// matching table
static struct of_device_id esl_audio_of_ids[] = {
  { .compatible = "esl,audio-fifo" },
  { }
};

// platform driver definition
static struct platform_driver esl_audio_driver = {
  .probe = esl_audio_probe,
  .remove = esl_audio_remove,
  .driver = {
    .name = DRIVER_NAME,
    .of_match_table = of_match_ptr(esl_audio_of_ids),
  },
};

static int esl_audio_init(void)
{
  int err;

  printk(KERN_INFO "zedaudio: started\n");

  // alocate character device region
  err = alloc_chrdev_region(&driver_data.first_devno, 0, 16, "zedaudio");
  if (err < 0)
    {
      return err;
    }

  // create class
  // although not using sysfs, still necessary in order to automatically
  // get device node in /dev
  driver_data.class = class_create(THIS_MODULE, "zedaudio");
  if (IS_ERR(driver_data.class))
    {
      return -ENOENT;
    }

  platform_driver_register(&esl_audio_driver);

  printk(KERN_INFO "zedaudio: Finished init\n");

  return 0;
}

static void esl_audio_exit(void)
{
  platform_driver_unregister(&esl_audio_driver);

  // free character device region
  unregister_chrdev_region(driver_data.first_devno, 16);

  // remove class
  class_destroy(driver_data.class);
}

module_init(esl_audio_init);
module_exit(esl_audio_exit);

MODULE_DESCRIPTION("ZedBoard Simple Audio driver");
MODULE_LICENSE("GPL");
