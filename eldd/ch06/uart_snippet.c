/* The UART operations structure */
static struct uart_opts usb_uart_ops = {
    .start_tx   =   usb_uart_start_tx,  /* Start transmitting */
    .startup   =   usb_uart_startup,  /* App opens USB_UART */
    .shutdown   =   usb_uart_shutdown,  /* App closes USB_UART */
    .type   =   usb_uart_type,  /* Set UART type */
    .config_port   =   usb_uart_config_port,  /* Configure when driver adds a USB_UART port */
    .request_port   =   usb_uart_request_port,  /* Claim resources associated with a USB_UART port */
    .release_port   =   usb_uart_release_port,  /* Release resources associated with a USB_UART port */
#if 0 /* Left unimplemented for the USB_UART */
    .tx_empty   =   usb_uart_tx_empty,  /* Transmitter busy? */
    .set_mctrl  =   usb_uart_set_mctrl, /* Set modem control */
    .get_mctrl  =   usb_uart_get_mctrl, /* Get modem control */
    .stop_tx  =   usb_uart_stop_tx, /* Stop transmission */
    .stop_rx  =   usb_uart_stop_rx, /* Stop reception */
    .enable_ms  =   usb_uart_enable_ms, /* Enable modem status signals */
    .set_termios  =   usb_uart_set_termios, /* Set termios */
#endif
};

static struct uart_driver usb_uart_reg = {
    .owner  =   THIS_MODULE,    /* Owner */
    .driver_name  =   "usb_uart",    /* Driver name */
    .dev_name  =   "ttyUU",    /* Node name */
    .major  =   USB_UART_MAJOR,    /* Major number */
    .minor  =   USB_UART_MINOR_START,    /* Minor number start */
    .nr  =   USB_UART_PORTS,    /* Number of UART ports */
    .cons  =   &usb_uart_console,    /* Pointer to the console structure. Discussed in Chapter 12, "Video Drivers" */
};

/* Called when the platform driver is unregistered */
staitc int usb_uart_remove(struct platform_device *dev)
{
    platform_set_drvdata(dev, NULL);

    /* Remove the USB_UART port from the serial core */
    uart_remove_one_port(&usb_uart_reg, &usb_uart_port[dev->id]);
    return 0;
}

/* Suspend power management event */
static int usb_uart_suspend(struct platform_device *dev, pm_message_t state)
{
    uart_suspend_port(&usb_uart_reg, &usb_uart_port[dev->id]);
    return 0;
}

/* Resume after a previous suspend */
static int usb_uart_resume(struct platform_device *dev)
{
    uart_resume_port(&usb_uart_reg, &usb_uart_port[dev->id]);
    return 0;
}

/* Parameters of each supported USB_UART port */
static sturct uart_port usb_uart_port[] = {
    {
        .mapbase    =   (unsigned int) USB_UART1_BASE,
        .iotype     =   UPID_MEM,   /* Memory mapped */
        .irq        =   USB_UART1_IRQ,   /* IRQ */
        .uartclk    =   USB_UART_CLK_FREQ,   /* Clock HZ */
        .filosize   =   USB_UART_FIFO_SIZE,   /* Size of the FIFO */
        .ops        =   &usb_uart_ops,   /* UART operations */
        .flags      =   UPF_BOOT_AUTOCONF,   /* UART port flag */
        .line       =   0,   /* UART port number */
    },
    {
        .mapbase    =   (unsigned int) USB_UART2_BASE,
        .iotype     =   UPID_MEM,   /* Memory mapped */
        .irq        =   USB_UART2_IRQ,   /* IRQ */
        .uartclk    =   USB_UART_CLK_FREQ,   /* Clock HZ */
        .filosize   =   USB_UART_FIFO_SIZE,   /* Size of the FIFO */
        .ops        =   &usb_uart_ops,   /* UART operations */
        .flags      =   UPF_BOOT_AUTOCONF,   /* UART port flag */
        .line       =   1,   /* UART port number */
    }
};


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

static platform_device *usb_uart_plat_device1; /* Platform device for USB_UART 1 */
static platform_device *usb_uart_plat_device2; /* Platform device for USB_UART 2 */

static struct platform_driver usb_uart_driver = {
    .probe  =   usb_uart_probe, /*  Probe method */
    .remove =   __exit_p(usb_uart_remove), /* Detach method */
    .suspend =   usb_uart_suspend, /* Power suspend */
    .resume =   usb_uart_resume, /* Resume after a suspend */
    .driver =   {
        .name = "usb_uart", /* Driver name */
    },
};

/* Driver Initialization */
static int __init usb_uart_init(void)
{
    int retval;

    /* Register the USB_UART driver with the serial core */
    if ((retval = uart_register_driver(&usb_uart_reg))) {
        return retval;
    }

    /* Register platform device for USB_UART 1. Usually called
     * during architecture-specific setup */
    usb_uart_plat_device1 = platform_device_register_simple("usb_uart", 0, NULL, 0);
    if (IS_ERR(usb_uart_plat_device1)) {
        uart_unregister_driver(&usb_uart_reg);
        return PTR_ERR(usb_uart_plat_device1);
    }


    /* Register platform device for USB_UART 2. Usually called 
     * during architecture-specific setup */
    usb_uart_plat_device2 = platform_device_register_simple("usb_uart", 1, NULL, 0);
    if (IS_ERR(usb_uart_plat_device2)) {
        uart_unregister_driver(&usb_uart_reg);
        platform_device_unregister(&usb_uart_plat_device1);
        return PTR_ERR(usb_uart_plat_device1);
    }

    /* Announce a matching driver for the platform
     * devices registered above */
    if ((retval = platform_driver_register(&usb_uart_driver))) {
        uart_unregister_driver(&usb_uart_reg);
        platform_device_unregister(usb_uart_plat_device1);
        platform_device_unregister(usb_uart_plat_device2);
    }
    return 0;
} 

/* Driver Exit */
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

module_init(usb_uart_init);
module_exit(usb_uart_exit);
