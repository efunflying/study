#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/pci.h>
#include <linux/input.h>
#include <linux/platform_device.h>

struct input_dev *vms_input_dev;    /* Representation of an input device */
static struct platform_device *vms_dev; /* Device structure */

/* Sysfs method to input simulated
 * coordinates to the virtual mouse driver */
static ssize_t write_vms(struct device *dev,
                         struct device_attribute *attr,
                         const char *buffer, size_t count)
{
    int x, y;
    sscanf(buffer, "%d%d", &x, &y);

    /* Report relative coordinates via the event interface */
    input_report_rel(vms_input_dev, REL_X, x);
    input_report_rel(vms_input_dev, REL_Y, y);
    input_sync(vms_input_dev);

    return count;
}

/* Attach the sysfs write method 
 * The result of this macro will define 
 * dev_attr_coordinates (dev_attr_ + <first param>) 
 * Here's how DEVICE_ATTR is defined:
 * #define    DEVICE_ATTR(_name, _mode, _show, _store) \
 * struct  device_attribute dev_attr_##_name = __ATTR(_name, _mode, _show, _store)
 *
 * */
DEVICE_ATTR(coordinates, 0644, NULL, write_vms);

/* Attribute Descriptor */
static struct attribute *vms_attrs[] = {
    &dev_attr_coordinates.attr,
    NULL
};

/* Attribute group */
static struct attribute_group vms_attr_group = {
    .attrs = vms_attrs,
};

/* Driver Initialization */
int __init vms_init(void)
{
    /* Register a platform device */
    vms_dev = platform_device_register_simple("vms", -1, NULL, 0);
    if (IS_ERR(vms_dev)) {
        printk("vms_init: error\n");
        return PTR_ERR(vms_dev);
    }

    /* Create a sysfs node to read simulated coordinates */
    sysfs_create_group(&vms_dev->dev.kobj, &vms_attr_group);

    /* Allocate an input device data structure */
    vms_input_dev = input_allocate_device();
    if (!vms_input_dev) {
        printk("Bad input_allocate_device()\n");
        return -ENOMEM;
    }

    /* Announce that the virtual mouse will generate relative coordinates */
    set_bit(EV_REL, vms_input_dev->evbit);
    set_bit(REL_X, vms_input_dev->relbit);
    set_bit(REL_Y, vms_input_dev->relbit);

    /* Register with the input subsystem */
    input_register_device(vms_input_dev);

    printk("Virtual Mouse Driver Initialized.\n");
    return 0;
}

/* Driver Exit */
void vms_cleanup(void)
{
    /* Unregister from the input subsystem */
    input_unregister_device(vms_input_dev);

    /* Cleanup sysfs node */
    sysfs_remove_group(&vms_dev->dev.kobj, &vms_attr_group);

    /* Unregister driver */
    platform_device_unregister(vms_dev);

    return;
}

module_init(vms_init);
module_exit(vms_cleanup);
