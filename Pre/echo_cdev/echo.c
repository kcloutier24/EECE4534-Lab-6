/* Pre-Lab 6: echo char driver */

#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/mutex.h>
#include <linux/kfifo.h>


#define MODULE_NAME "chr-echo"

/* fifo size in elements (bytes) */
#define FIFO_SIZE	32

/* name of the proc entry */
#define	PROC_FIFO	"bytestream-fifo"

/* lock for procfs read access */
static DEFINE_MUTEX(read_lock);

/* lock for procfs write access */
static DEFINE_MUTEX(write_lock);

/*
 * define DYNAMIC in this example for a dynamically allocated fifo.
 *
 * Otherwise the fifo storage will be a part of the fifo structure.
 */
#if 0
#define DYNAMIC
#endif

#ifdef DYNAMIC
static struct kfifo test;
#else
static DECLARE_KFIFO(test, unsigned char, FIFO_SIZE);
#endif




static dev_t echo_devno;
static struct cdev echo_dev;
static struct class *echo_class;





/* 
Old 

static ssize_t echo_read(struct file *f,
                         char __user *buf,
                         size_t count,
                         loff_t *offset)
{
  int ret = 0;
  return ret;
}


static ssize_t echo_write(struct file *f,
                          const char __user *buf,
                          size_t count,
                          loff_t *offset)
{

  int ret = 0;
  
  /*
  Warning

  must be 32 characters or kernel will crash
  
  char print_buf[32]; // Create a local character array of 32 bytes (call it e.g. print_buf).

  if(count> sizeof(print_buf))
  {

    printk("That is too big %d to fit into %d", count, sizeof(print_buf));
    return -EINVAL;
  }

*/

//Improve the implementation so that it can handle long messages of any size.
//char *print_buf;// to handle long messages i implemented dynamic memory allocation to allow for the flexibility in accommodating a message of great length
/*
print_buf= kmalloc(count, GFP_KERNEL);
if(!print_buf)
{
  return -ENOMEM;
}



  // Copy the data from user space into kernel space
  //  to from number of bytes
  if (copy_from_user(print_buf, buf, count))
  {
    printk("Failed to write to %sfrom %s \n", print_buf, buf);

    return -EFAULT;
  }
  else
  {

   printk("Received meddage: %s \n", print_buf);
/*
    int i =0;
    while (i<sizeof(print_buf))
    {
printk("%c", print_buf[i]);

i++;
    }
    printk("\n");
    */


 // }

  /*
Warning!

ensure proper error handlling and resource management
negative return values are errors

*/

  //return ret;
//}






static ssize_t echo_read(struct file *f,
                         char __user *buf,
                         size_t count,
                         loff_t *offset)
{
  //6

	int ret;
	unsigned int copied;

	if (mutex_lock_interruptible(&read_lock))
  {
      printk("errorrrrrrrrr ");

		return -ERESTARTSYS;
}
	ret = kfifo_to_user(&test, buf, 32, &copied);

	mutex_unlock(&read_lock);
  printk("retttttttttttttttttttttt %d", ret);
    printk("coppiedddddddddddddddddddd %d", copied);


	return ret ? ret : copied;

}


static ssize_t echo_write(struct file *f,
                          const char __user *buf,
                          size_t count,
                          loff_t *offset)
{

  //6 
	int ret;
	unsigned int copied;

	if (mutex_lock_interruptible(&write_lock))
		return -ERESTARTSYS;

	ret = kfifo_from_user(&test, buf, 32, &copied);

    printk("retttttttttttttttttttttt %d", ret);
    printk("coppiedddddddddddddddddddd %d", copied);

	mutex_unlock(&write_lock);

	return ret ? ret : copied;
}











struct file_operations echo_fops = {
    .read = echo_read,
    .write = echo_write,
};

static int echo_init(void)
{
  int err;

  // TODO comment 1
  /*

  initializes a character device region
  allocates a range of device numbers dynamically for the character device
  alloc_chrdev_region() assigns major and minor numbers to the driver

  */

  err = alloc_chrdev_region(&echo_devno, 0, 1, "echo");
  if (err)
  {
    return err;
  }

  // TODO comment 2
  /*

  initializes the echo_dev structure with the file operations defined in echo_fops
  struct represents our character device.

  */
  cdev_init(&echo_dev, &echo_fops);

  // TODO comment 3
  /*

  adds the character device represented by echo_dev to the system
device is made available to the system
and the driver is associated with the corresponding major and minor numbers obtained in the echo_devno variable

  */
  err = cdev_add(&echo_dev, echo_devno, 1);
  if (err)
  {
    unregister_chrdev_region(echo_devno, 1);
    return err;
  }

  // create class to which this echo device belongs
  // (needed for device_create even though we have single instance)
  echo_class = class_create(THIS_MODULE, "echo_char");
  if (IS_ERR(echo_class))
  {
    return -ENOENT;
  }

  // TODO comment 4
  /*

  creates a device file in the /dev directory associated with the character device
  device_create() function
    passing the device class echo_class
    the parent device NULL, since there isn't any
     the device number echo_devno
      additional data NULL
       and the device name "echo0"
  This step is necessary for user-space programs to interact with the driver.

  */
  device_create(echo_class, NULL, echo_devno, NULL, "echo0");

  // TODO add tracing for successful char def creation
  /*
  Add error handling to the device_create() call,
  print a trace message upon successful initialization with the device major and minor number
  */
  if (IS_ERR(echo_class))
  {
    printk("Failed to create device file\n");
    return PTR_ERR(echo_class);
  }
  else
  {
    printk("Character device initialized (major = %d, minor = %d)\n", MAJOR(echo_devno), MINOR(echo_devno));
  }

  return 0;
}

static void echo_exit(void)
{
  // delete char device and driver
  cdev_del(&echo_dev);
  device_destroy(echo_class, echo_devno);

  // delete class
  class_destroy(echo_class);

  // unregister chrdev region
  unregister_chrdev_region(echo_devno, 1);
}

module_init(echo_init);
module_exit(echo_exit);

MODULE_DESCRIPTION("Echo Character device");
MODULE_LICENSE("GPL");
