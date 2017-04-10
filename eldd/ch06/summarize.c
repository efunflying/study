// buttom layer:  uart driver
// Main skeleton for init: 
// 1. uart_register_driver (Register the USB_UART driver with the serial core)
// 2. platform_device_register_simple (Register platform device for USB_UART)
// 3. platform_driver_register (Announce a matching driver for the platform devices registered above)
// Main skeleton for exit is reversed with init.

static struct uart_driver usb_uart_reg = {
    .owner  =   THIS_MODULE,    /* Owner */
    .driver_name  =   "usb_uart",    /* Driver name */
    .dev_name  =   "ttyUU",    /* Node name */
    .major  =   USB_UART_MAJOR,    /* Major number */
    .minor  =   USB_UART_MINOR_START,    /* Minor number start */
    .nr  =   USB_UART_PORTS,    /* Number of UART ports */
    .cons  =   &usb_uart_console,    /* Pointer to the console
                                        structure. Discussed in Chapter
                                        12, "Video Drivers" */
};

static struct platform_driver usb_uart_driver = {
    .probe  =   usb_uart_probe, /*  Probe method */
    .remove =   __exit_p(usb_uart_remove), /* Detach method */
    .suspend =  usb_uart_suspend, /* Power suspend */
    .resume =   usb_uart_resume, /* Resume after a suspend */
    .driver =   {
        .name = "usb_uart", /* Driver name */
    },
};

static platform_device *usb_uart_plat_device1; /* Platform device for USB_UART 1 */
static platform_device *usb_uart_plat_device2; /* Platform device for USB_UART 2 */

static int __init module_init_func(void)
{
    /* Register the USB_UART driver with the serial core */
    uart_register_driver(&usb_uart_reg);

    /* Register platform device for USB_UART 1. Usually called
     * during architecture-specific setup */
    usb_uart_plat_device1 = platform_device_register_simple("usb_uart", 0, NULL, 0);
    /* Register platform device for USB_UART 2. Usually called 
     * during architecture-specific setup */
    usb_uart_plat_device2 = platform_device_register_simple("usb_uart", 1, NULL, 0);

    /* Announce a matching driver for the platform
     * devices registered above */
    platform_driver_register(&usb_uart_driver);

    return 0;
} 


static void __exit usb_uart_exit(void)
{
    /* The order of unregisteration is important. Unregistering the 
     * UART driver before the platform driver will crash the system */

    /* Unregister the platform driver */
    platform_driver_unregister(&usb_uart_driver);

    /* Unregister the platform devices */
    platform_device_unregister(usb_uart_plat_device1);
    platform_device_unregister(usb_uart_plat_device2);

    /* Unregister the USB_UART driver */
    uart_unregister_driver(&usb_uart_reg);
}

/* There are two important steps that a UART driver has to do to tie itself with the kernel: 
 * 1. Register with the serial core by calling
 * uart_register_driver(struct uart_driver *);
 * 2. Invoke uart_add_one_port(struct uart_driver * , struct uart_port *) to register
 * each individual port that it supports. If your serial hardware is hotplugged, the ports
 * are registered with the core from the entry point that probes the presence of the device. 
 */

/* Platform driver probe */
static int __init usb_uart_probe(struct platform_device *dev)
{
    /* ... */

    /* Add a USB_UART port. This function also registers this device
     * with the tty layer and triggers invocation of the config_port()
     * entry point */
    uart_add_one_port(&usb_uart_reg, &usb_uart_port[dev->id]);
    platform_set_drvdata(dev, &usb_uart_port[dev->id]);
    return 0;
}



// Middle Layer: tty driver
