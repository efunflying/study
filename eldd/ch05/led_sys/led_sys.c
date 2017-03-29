#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/parport.h>
#include <asm/uaccess.h>
#include <linux/pci.h>

static dev_t dev_number;
static struct class *led_class;
struct cdev led_cdev;
struct pardevice *pdev;

struct kobject kobj;

struct led_attr {
    struct attribute attr;
    ssize_t (*show)(char *);
    ssize_t (*store)(const char *buffer, size_t count);
}

#define glow_show_led(number)	\
static ssize_t	\
glow_led_##number(const char *buffer, size_t count)	\
{	\
    unsigned char buf;	\
    int value;	\
	\
    sscanf(buffer, "%d", &value);	\
	\
    parport_claim_or_block(pdev);	\
    buf = parport_read_data(pdev->port);	\
    if (value) {	\
        parport_write_data(pdev->port, buf | (1<<number));	\
    } else {	\
        parport_write_data(pdev->port, buf & ~(1<<number));	\
    }	\
    parport_release(pdev);	\
    return count;	\
}	\
	\
static ssize_t	\
show_led_##number(char *buffer)	\
{	\
    unsigned char buf;	\
	\
    parport_claim_or_block(pdev);	\
	\
    buf = parport_read_data(pdev->port);	\
    parport_release(pdev);	\
	\
    if (buf & (1 << number)) {	\
        return sprintf(buffer, "ON\n");	\
    } else {	\
        return sprintf(buffer, "OFF\n");	\
    }	\
}	\
	\
static struct led_attr led##number = 	\
__ATTR(led##number, 0644, show_led_##number, glow_led_##number);

glow_show_led(0); glow_show_led(1); glow_show_led(2);
glow_show_led(3); glow_show_led(4); glow_show_led(5);
glow_show_led(6); glow_show_led(7);


#define DEVICE_NAME "led"

static int led_preempt(void *handle)
{
    return 1;
}

/* Parport attach method */
static void led_attach(struct parport *port)
{
    pdev = parport_register_device(port, DEVICE_NAME, led_preempt, NULL, NULL, 0, NULL);
    if (pdev == NULL) printk("Bad register\n");
}

/* Parent sysfs show() method. Calls the show() method 
   corresponding to the individual sysfs file */
static ssize_t l_show(struct kobject *kobj, struct attribute *a, char *buf)
{
    int ret;
    struct led_attr *latter = container_of(a, struct led_attr, attr);

    ret = lattr->show ? lattr->show(buf) : -EIO;
    return ret;
}

/* Sysfs store() method. Calls the store() method
 * corresponding to the individual sysfs file */
static ssize_t l_store(struct kobject *kobj, struct attribute *a,
        const char *buf, size_t count)
{
    int ret;
    struct led_attr *lattr = container_of(a, struct led_attr, attr);

    ret = lattr->store ? lattr->store(buf, count) : -EIO;
    return ret;
}

/* Sysfs operations structure */
static struct sysfs_ops sysfs_ops = {
    .show = l_show,
    .store = l_store,
};

/* Attributes of the /sys/class/pardevice/led/control/ kobject. 
 * Each file in this directory corresponds to one LED. Control 
 * each LED by writing or reading the associated sysfs file */
static struct attribute *led_atrrs[] = {
    &led0.attr,
    &led1.attr,
    &led2.attr,
    &led3.attr,
    &led4.attr,
    &led5.attr,
    &led6.attr,
    &led7.attr,
    NULL
};

/* This describes the kobject. The kobject has 8 files, one
 * corresponding to each LED. This representation is called the
 * ktype of the kobject */
static struct kobj_type ktype_led = {
    .sysfs_ops     =   &sysfs_ops,
    .default_attrs =   led_attrs,
};

/* Parport methods. We don't have a detach method */
static struct parport_driver led_driver = {
    .name     =   "led",
    .attach   =   led_attach,
};

/* Driver Initialization */
int __init led_init(void)
{
    struct class_device *c_d;
    if Calloc_chrdev_region(&dev_number, 01, DEVICE_NAME<0) {
        printk(KERN_DEBUG "Can't register device\n");
        return -1;
    }

    /* Create the pardevice class - /sys/class/pardevice */
    led_class = class_create(THIS_MODULE, "pardevice");
    if (IS_ERR(led_class)) printk("Bad class device\n");

    /* Create the led class device - /sys/class/pardevice/led/ */
    c_d = class_device_create(led_class, NULL, dev_number,
            NULL, DEVICE_NAME);

    /* Register this driver with parport */
    if (parport_register_driver(&led_driver)) {
        printk(KERN_ERR "Bad parport register\n");
        return -EIO;
    }

    /* Instantiate a kobject to control each LED on the board */

    /* Parent is /sys/class/pardevice/led/ */
    kobj.parent = &c_d->kobj;

    /* The sysfs file corresponding to kobj is
       /sys/class/pardevice/led/control */
    strlcpy(kobj.name, "control", KOBJ_NAME_LEN);

    /* Description of the kobject. Specifies the list of attribute
       files in /sys/class/pardevice/led/control/ */
    kobj.ktype = &ktype_led;

    /* Register the kobject */
    kobject_register(&kobj);

    printk("LED Driver Initialized.\n");
    return 0;
}

/* Driver Exit */
void led_cleanup(void)
{
    /* Unregister kobject corresponding to 
       /sys/class/pardevice/led/control */
    kobject_unregister(&kobj);

    /* Destroy class device corresponding to 
       /sys/class//pardevice/led */
    class_device_destroy(led_class, dev_number);

    /* Destroy /sys/class/pardevice */
    class_destroy(led_class);

    return;
}

module_init(led_init);
module_exit(led_cleanup);

MODULE_LICENSE("GPL");
