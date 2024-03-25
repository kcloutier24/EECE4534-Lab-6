/* LAB 3 PWM Controller kernel module reference implementation */
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sysfs.h>
#include <linux/of_device.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <asm/io.h>

// define driver/module name
#define DRIVER_NAME "axitimer"

// INSTANCE flags
#define INSTANCE_FLAG_RUNNING 0x01

// maximum number of instances
#define MAX_INSTANCE_NUMBER 16
#define TIMER_MAX_COUNT 0xFFFFFFFF

// ###################  AXI TIMER Specific Definitions and Functions ################## // 
//#include <asm/segment.h>
//#include <asm/uaccess.h>
//#include <linux/buffer_head.h>

#define UDT_MASK   (1<<1) //Define the necessary bits used for configuring the timer to PWM mode
#define GENT_MASK  (1<<2)
#define ARHT_MASK  (1<<4)
#define LOAD_MASK  (1<<5)
#define ENT_MASK   (1<<7)
#define PWMA0_MASK (1<<9)
#define ENALL_MASK (1<<10)
#define BASIC_CONFIG_MASK (UDT_MASK|GENT_MASK|ARHT_MASK|PWMA0_MASK)

#define TCSR0_OFFSET 0x0 //Register offsets
#define TCSR1_OFFSET 0x10
#define TLR0_OFFSET 0x04
#define TLR1_OFFSET 0x14
#define TCR0_OFFSET 0x08
#define TCR1_OFFSET 0x18

#define TIMER_FREQUENCY 100000000 //1Mhz timer speed
#define AXI_CLOCK_PERIOD 1.0/TIMER_FREQUENCY

// #######################################################################################

// define structure for device instance data
struct esl_axitimer_instance
{
  void __iomem *regs; // memory-mapped registers
  unsigned int flags; // instance flags
  dev_t devno; // device number
  unsigned long period;
  unsigned long duty;
};

// define structure for class data (common across all instances)
struct esl_axitimer_global
{
  u32 dev_major; //device major number
  dev_t devt;

  unsigned int instance_number; // ever-increasing number for instances
};

// Instantiate class data 
static struct esl_axitimer_global driver_data;

// Utility method to read registers
static inline u32 reg_read(struct esl_axitimer_instance* inst, u32 reg)
{
  return ioread32(inst->regs + reg);
}

// Utility method to write registers
static inline void reg_write(struct esl_axitimer_instance* inst, u32 reg,
                             u32 value)
{
  iowrite32(value, inst->regs + reg);
}

// initialize device
static inline void axitimer_initialize(struct esl_axitimer_instance* inst)
{
  //Add register operations to create the PWM device
  reg_write(inst, TCSR0_OFFSET, 0); //Set the correct configuration for timer0
  reg_write(inst, TCSR0_OFFSET, BASIC_CONFIG_MASK);

  reg_write(inst, TCSR1_OFFSET, 0); //Set the correct configuration for timer1
  reg_write(inst, TCSR1_OFFSET, BASIC_CONFIG_MASK);

  printk(KERN_INFO "kPWM: Successfully initialized PWM values");
}

static inline void axitimer_start(struct esl_axitimer_instance* inst)
{
  //Write the period and duty cycle into the timers
  u32 TLR0 = (inst->period * 100) - 2;
  u32 TLR1 = (inst->period * inst->duty);
  reg_write(inst, TLR0_OFFSET, TLR0); 
  reg_write(inst, TLR1_OFFSET, TLR1);

  //Load the value into the register and clear it
  reg_write(inst, TCSR0_OFFSET, BASIC_CONFIG_MASK | LOAD_MASK); 
  reg_write(inst, TCR0_OFFSET, BASIC_CONFIG_MASK);

  //Set the enable all bit and then clear it without clearing the Enable timer mask
  reg_write(inst, TCSR0_OFFSET, BASIC_CONFIG_MASK | ENALL_MASK);
  reg_write(inst, TCSR0_OFFSET, BASIC_CONFIG_MASK | ENT_MASK);

  printk(KERN_INFO "kPWM: PWM Timer Started\n");
}

static inline void axitimer_updateTimer(struct esl_axitimer_instance* inst)
{
  //Write the period and duty cycle into the timers
  u32 TLR0 = (inst->period * 100) - 2;
  u32 TLR1 = (inst->period * inst->duty);
  reg_write(inst, TLR0_OFFSET, TLR0); 
  reg_write(inst, TLR1_OFFSET, TLR1);

  //Load the value into the register and clear it
  reg_write(inst, TCSR0_OFFSET, BASIC_CONFIG_MASK | LOAD_MASK); 
  reg_write(inst, TCR0_OFFSET, BASIC_CONFIG_MASK);

  //Set the enable all bit and then clear it without clearing the Enable timer mask
  reg_write(inst, TCSR0_OFFSET, BASIC_CONFIG_MASK | ENALL_MASK);
  reg_write(inst, TCSR0_OFFSET, BASIC_CONFIG_MASK | ENT_MASK);
}

static inline void axitimer_stop(struct esl_axitimer_instance* inst)
{
  reg_write(inst, TCSR0_OFFSET, BASIC_CONFIG_MASK);
  reg_write(inst, TCSR1_OFFSET, BASIC_CONFIG_MASK);

  printk(KERN_INFO "kPWM: PWM Timer Stopped\n");
}

// This function is called by the status file and prints idle when the PWM controller is IDLE
static ssize_t esl_axitimer_state_show(struct device* dev,
                                       struct device_attribute* attr,
                                       char* buf)
{
  struct esl_axitimer_instance* inst = dev_get_drvdata(dev);

  if (inst->flags & INSTANCE_FLAG_RUNNING)
    {
      return sprintf(buf, "running\n");
    }

  return sprintf(buf, "idle\n");
}


//This function is defined and linked to writing to the control file that is created, and is called when something is written to that file.
static ssize_t esl_axitimer_control_store(struct device* dev,
                                          struct device_attribute* attr,
                                          const char* buf, size_t count)
{
  struct esl_axitimer_instance* inst = dev_get_drvdata(dev);

  if (!strncmp(buf, "on\n", count))
    {
      inst->flags |= INSTANCE_FLAG_RUNNING;
      axitimer_start(inst);
    }
  else if (!strncmp(buf, "off\n", count))
    {
      inst->flags &= ~INSTANCE_FLAG_RUNNING;
      axitimer_stop(inst);
    }
  else
    {
      return -EINVAL;
    }

  return count;
}

//This function is defined and linked to writing to the period file that is created, and is called when something is written to that file.
//to use use the command: "echo 100 > store" to make the period 100
static ssize_t esl_axitimer_period_store(struct device* dev,
                                          struct device_attribute* attr,
                                          const char* buf, size_t count)
{
  struct esl_axitimer_instance* inst = dev_get_drvdata(dev);

  unsigned long num = -1;
  int status = kstrtoul(buf, 0, &num);

  if (num > 0 && num < 0xFFFFFFFF && status == 0)
    {
      inst->period = num;
      axitimer_updateTimer(inst);
    }
  else
    {
      printk(KERN_ERR "kPWM: %ld not written to period", num);
      return -EINVAL;
    }

  return count;
}

// This function is called by the period file when it is being read and prints the period
// use the command to show the period: cat period
static ssize_t esl_axitimer_period_show(struct device* dev,
                                       struct device_attribute* attr,
                                       char* buf)
{
  struct esl_axitimer_instance* inst = dev_get_drvdata(dev);
  return sprintf(buf, "Period: %ld\n", inst->period);
}


//This function is defined and linked to writing to the period file that is created, and is called when something is written to that file.
//To use use the command: echo 20 > duty..... will set duty cycle to 20%
static ssize_t esl_axitimer_duty_store(struct device* dev,
                                          struct device_attribute* attr,
                                          const char* buf, size_t count)
{
  struct esl_axitimer_instance* inst = dev_get_drvdata(dev);

  unsigned long num = -1;
  int status = kstrtoul(buf, 0, &num);

  if (num >= 0 && num <= 100 && status == 0)
    {
      inst->duty = num;
      axitimer_updateTimer(inst);
    }
  else
    {
      printk(KERN_ERR "kPWM: %ld not written to duty", num);
      return -EINVAL;
    }

  return count;
}

// This function is called by the period file when it is being read and prints the period
//to print out use the command: cat duty
static ssize_t esl_axitimer_duty_show(struct device* dev,
                                       struct device_attribute* attr,
                                       char* buf)
{
  struct esl_axitimer_instance* inst = dev_get_drvdata(dev);
  return sprintf(buf, "Duty Cycle: %ld%%\n", inst->duty);
}

// define our own class for proper and automatic sysfs file group creation


// start / stop control
static struct device_attribute esl_axitimer_control = {
  .attr = {
    .name = "control",
    .mode = S_IWUSR, // write only
  },
  .store = esl_axitimer_control_store,
};

// current state
static struct device_attribute esl_axitimer_state = {
  .attr = {
    .name = "status",
    .mode = S_IRUGO, 
  },
  .show = esl_axitimer_state_show,
};

// PWM period
static struct device_attribute esl_axitimer_period = {
  .attr = {
    .name = "period",
    .mode = S_IRWXUGO, 
  },
  .show = esl_axitimer_period_show,
  .store = esl_axitimer_period_store,
};

// PWM duty cycle
static struct device_attribute esl_axitimer_duty = {
  .attr = {
    .name = "duty",
    .mode = S_IRWXUGO, 
  },
  .show = esl_axitimer_duty_show,
  .store = esl_axitimer_duty_store,
};

// array of attributes
static struct attribute* esl_axitimer_attrs[] = {
  &esl_axitimer_control.attr,
  &esl_axitimer_state.attr,
  &esl_axitimer_period.attr,
  &esl_axitimer_duty.attr,
  NULL
};

// Group all attributes to "control" folder, 
// this creates /sys/class/axitimer/axitimerX/control
static struct attribute_group esl_axitimer_attr_group = {
  .name = "control", // control folder
  .attrs = esl_axitimer_attrs,
};

// array of attribute groups (sysfs files and folders)
static const struct attribute_group* esl_axitimer_attr_groups[] = {
  &esl_axitimer_attr_group, // the "control" folder
  NULL
};

// define the class, creates /sys/class/axitimer
static struct class esl_axitimer_class = {
  .name = "axitimer",
  .owner = THIS_MODULE,
};


// probe, or add an instance
static int esl_axitimer_probe(struct platform_device* pdev)
{
  struct esl_axitimer_instance* inst = NULL; // new instance
  struct resource* res; // for reading resources from device tree
  struct device* dev;
  const void* property;
  // make new device; we need device a number (major, minor)
  // use instance count as the minor, which we increase each time
  // major has already been allocated in init function
  // to take a look at majors, do cat /proc/devices
  dev_t devno = MKDEV(driver_data.dev_major, driver_data.instance_number);

  // verify if timer is marked as PWM timer, get resource from device tree
  property = of_get_property(pdev->dev.of_node, "esl,pwm-timer", NULL);
  if (!property)
    {
      // timer is not marked for PWM, fail probe
      printk(KERN_INFO "kPWM: %s not marked as PWM timer.\n",
             pdev->name);
      printk(KERN_WARNING "kPWM: Error skipped for debugging purposes\n");
      //return -EPERM;
    }
    else
    {
      printk(KERN_INFO "kPWM: %s correctly marked as PWM timer.\n",
             pdev->name);
    }

  //allocate new instance. explanation for this:
  // 1. we use devm_kzalloc instead of kzalloc because it has the nice property
  //    of freeing unused resources automatically when the platform driver is
  //    removed (devm = dev managed and attached to pdev->dev so when it is
  //    destroyed, data is garbage collected)
  // 2. GFP_KERNEL means we are allocating KERNEL memory, otherwise it should be
  //    GFP_USER
  inst = devm_kzalloc(&pdev->dev, sizeof(struct esl_axitimer_instance),
                      GFP_KERNEL);

  if (!inst)
    {
      // ran out of memory
      return -ENOMEM;
    }

  //This function gets the memory structure of a device resource in order to map it in memory
  //The IOSOURCE_MEM is to handle the memory/mapping memory
  //It is needed in order to find out where in memory the timers are mapped because it may vary from system to system.
  res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
  if (IS_ERR(res))
    {
      return PTR_ERR(res);
    }

  //This function checks the requested memory region, requests it from the kernel, and remaps it
  //It is necessary to properly map the timers to memory we as the kernel module can use
  inst->regs = devm_ioremap_resource(&pdev->dev, res);
  if (IS_ERR(inst->regs))
    {
      // error mapping
      return PTR_ERR(inst->regs);
    }

  // set platform driver data (our instance struct)
  platform_set_drvdata(pdev, inst);

  // create the character device, this will create:
  // 1. /dev/axitimerX
  // 2. all the sysfs stuff
  dev = device_create_with_groups(&esl_axitimer_class, &pdev->dev,
                                  devno, inst, esl_axitimer_attr_groups,
                                  "axitimer%d", driver_data.instance_number);

  if (IS_ERR(dev))
    {
      return PTR_ERR(dev);
    }

  // set dev number
  inst->devno = devno;

  // increment instance counter
  driver_data.instance_number++;

  //Set default period (100000us)
  inst->period = 1000000;

  //Set default duty cycle (50%)
  inst->duty = 50;

  // initialize timer
  axitimer_initialize(inst);

  printk(KERN_INFO "probed kPWM with timer %s\n", pdev->name);

  return 0;
}

// remove an instance
static int esl_axitimer_remove(struct platform_device* pdev)
{
  struct esl_axitimer_instance* inst = platform_get_drvdata(pdev);

  //Disable timers
  axitimer_stop(inst);

  // cleanup and remove
  device_destroy(&esl_axitimer_class, inst->devno);

  return 0;
}

// matching table
// list of strings that indicate what platform
// device is compatible with
static struct of_device_id esl_axitimer_of_ids[] = {
  { .compatible = "xlnx,xps-timer-1.00.a" },
  {}
};

// platform driver definition
static struct platform_driver esl_axitimer_driver = {
  .probe = esl_axitimer_probe,
  .remove = esl_axitimer_remove,
  .driver = {
    .name = DRIVER_NAME,
    .of_match_table = of_match_ptr(esl_axitimer_of_ids),
  },
};

// module initialization
static int esl_axitimer_init(void)
{
  int err;

  // allocate character device region, /dev/axitimerX
  err = alloc_chrdev_region(&driver_data.devt, 0, MAX_INSTANCE_NUMBER,
                            DRIVER_NAME);
  if (err)
    {
      return err;
    }

  // initialize our data
  driver_data.instance_number = 0;
  driver_data.dev_major = MAJOR(driver_data.devt);

  //register class
  class_register(&esl_axitimer_class);

  //register platform driver
  platform_driver_register(&esl_axitimer_driver);

  return 0;
}

// module removal
static void esl_axitimer_exit(void)
{
  //cleanup
  platform_driver_unregister(&esl_axitimer_driver);
  class_unregister(&esl_axitimer_class);
  unregister_chrdev_region(driver_data.devt, MAX_INSTANCE_NUMBER);
}






module_init(esl_axitimer_init);
module_exit(esl_axitimer_exit);

MODULE_DESCRIPTION("AXI Timer driver");
MODULE_LICENSE("GPL");