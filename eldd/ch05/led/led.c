#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/parport.h>
#include <linux/platform_device.h>
#include <asm/uaccess.h>

#define DEVICE_NAME "led"

static dev_t dev_number;
static struct class *led_class;
struct cdev led_cdev;
struct pardevice *pdev;

int led_open(struct inode *inode, struct file *file)
{
    return 0;
}

ssize_t led_write(struct file *file, const char *buf, 
        size_t count, loff_t *ppop)
{
    char kbuf;

    if (copy_from_user(&kbuf, buf, 1)) return -EFAULT;

    /* Claim the port*/
    parport_claim_or_block(pdev);

    /* Write to the device */
    parport_write_data(pdev->port, kbuf);

    /* Release the port */
    parport_release(pdev);

    return count;
}

/* Release the device */
int led_release(struct inode *inode, struct file *file)
{
    return 0;
}

static struct file_operations led_fops = {
    .owner = THIS_MODULE,
    .open = led_open,
    .write = led_write,
    .release = led_release,
};

static int led_preempt(void *handle)
{
    return 1;
}

/* Parport attach method */
static void led_attach(struct parport *port)
{
    /* Register the parallel LED device with parport */
    pdev = parport_register_device(port, DEVICE_NAME, led_preempt, NULL, NULL, 0, NULL);
    if (pdev == NULL) printk("Bad register\n");
}

/* Parport detach method */
static void led_detach(struct parport *port)
{
    /* do nothing */
}

/* Parport driver operations */
static struct parport_driver led_driver = {
    .name = "led",
    .attach = led_attach,
    .detach = led_detach,
};


/* Driver Initialization */
int __int led_init(void)
{
    /* Request dynamic allocation of a device major number */
    if (alloc_chrdev_region(&dev_number, 0, 1, DEVICE_NAME)
            < 0) {
        printk(KERN_DEBUG "Can't register device\n");
        return -1;
    }

    /* Create the led class */
    led_class = class_create(THIS_MODULE, DEVICE_NAME);
    if (IS_ERR(led_class)) printk("Bad class create\n");

    /* Connect the file operations with the cdev */
    cdev_init(&led_cdev, &led_fops);

    led_cdev.owner = THIS_MODULE;

    /* Connect the major/minor number to the cdev */
    if (cdev_add(&led_cdev, dev_number, 1)) {
        printk("Bad cdev add\n");
        return 1;
    }

    class_device_create(led_class, NULL, dev_number, NULL, DEVICE_NAME);

    /* Register this driver with parport */
    if (parport_register_driver(&led_driver)) {
        printk(KERN_ERR "Bad Parport Register\n");
        return -EIO;
    }

    printk("LED Driver Initialized.\n");

    return 0;
}

/* Driver Exit */
void __exit led_cleanup(void)
{
    unregister_chrdev_region(dev_number, 1);
    class_device_destroy(led_class, dev_number);
    class_destroy(led_class);
    return;
}

module_init(led_init);
module_exit(led_cleanup);

MODULE_LICENSE("GPL");
