#include <linux/console.h>
#include <linux/platform_device.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/serial_core.h>
#include <linux/serial.h>
#include <asm/irq.h>
#include <asm/io.h>

#define USB_UART_MAJOR	200	/* You have to get this assigned */
#define USB_UART_MINOR_START	70	/* Start minor numbering here */
#define USB_UART_PORT	2	/* The phone has 2 USB_UARTS */
#define PORT_USB_UART	30	/* UART type. Add this to include/linux/serial_core.h*/


/* Each USB_UART has a 3-byte register set consisting of
 * UU_STATUS_REGISTER at offset 0, UU_READ_DATA_REGISTER at
 * offset 1, and UU_WRITE_DATA_REGISTER at offset 2 as shown
 * in Table 6.1 */
#define USB_UART1_BASE	0xe8000000	/* Memory base for USB_UART1 */
#define USB_UART2_BASE	0xe9000000	/* Memory base for USB_UART2 */
#define USB_UART_REGISTER_APACE	0x3

/* Semantics of bits in the status register */
#define USB_UART_TX_FULL	0x20	/* TX FIFO is full */
#define USB_UART_RX_EMPTY	0x10	/* TX FIFO is empty */
#define USB_UART_STATUS		0x0F	/* Parity/frame/overruns? */

#define USB_UART1_IRQ	3	/* USB_UART1 IRQ */
#define USB_UART2_IRQ	4	/* USB_UART2 IRQ */
#define USB_UART_FIFO_SIZE	32	/* FIFO size */
#define USB_UART_CLK_FREQ	16000000

static struct uart_port usb_uart_port[]; /* Defined later on */

/* Write a character to the USB_UART port */
static void usb_uart_putc(struct uart_port *port, unsigned char c)
{
     /* Write untile there is space in the TX FIFO of the USB_UART.
      * Sense this by looking at the USB_UART_TX_FULL bit in the 
      * status register */
    while (__raw_readb(port->membase) & USB_UART_TX_FULL);

    /* Write the character to the data port */
    __raw_writeb(c, (port->membase+1));
}

/* Read a character from the USB_UART */
static unsigned char usb_uart_getc(struct uart_port *port)
{
    /* Wait until data is available in the RX_FIFO */
    while (__raw_readb(port->membase) & USB_UART_RX_EMPTY);

    /* Obtain the data */
    return (__raw_readb(port->membase+2));
}

/* Obtain USB_UART status */
static unsigned char usb_uart_status(struct uart_port *port)
{
    return (__raw_readb(port->membase) & USB_UART_STATUS);
}

/*
 * Claim the memory region attached to USB_UART port. Called 
 * when the driver adds a USB_UART port via uart_add_one_port().
 */
static int usb_uart_request_port(struct uart_port *port)
{
    if (!request_mem_region(port->mapbase, USB_UART_REGISTER_SPACE,
                "usb_uart")) {
        return -EBUSY;
    }
    return 0;
}

/* Release the memory region attached to USB_UART port.
 * Called when the driver removes a USB_UART port via
 * uart_remove_one_port().
 */
static void usb_uart_release_port(struct uart_port *port)
{
    release_mem_region(port->mapbase, USB_UART_REGISTER_SPACE);
}

/*
 * Configure USB_UART. Called when the driver adds a USB_UART port.
 */
static void usb_uart_config_port(struct uart_port *port, int flags)
{
    if (flag & UART_CONFIG_TYPE && usb_uart_request_port(port) == 0)
    {
        port->type = PORT_USB_UART;
    }
}

/* Receive interrupt handler */
static irqreturn_t usb_uart_rxint(int irq, void *dev_id)
{
    struct uart_port *port = (struct uart_port *) dev_id;
    struct tty_struct *tty = port->info->tty;

    unsigned int status, data;
    /* ... */
    do {
        /* ... */
        /* Read data */
        data = usb_uart_getc(port);
        /* Normal, overrun, parity, frame error? */
        status = usb_uart_status(port);
        /* Dispatch to the tty layer */
        tty_insert_flip_char(tty, data, status);
        /* ... */
    } while (more_chars_to_be_read()); /* More chars */
    /* ... */
    tty_flip_buffer_push(tty);

    return IRQ_HANDLED;
}

/* Called when an application opens a USB_UART */
static void usb_uart_startup(struct uart_port *port)
{
    int retval = 0;
    /* ... */
    /* Request IRQ */
    if ((retval = request_irq(port->irq, usb_uart_rxint, 0,
                    "usb_uart", (void *)port))) {
        return retval;
    }
    /* ... */
    return retval;
}

/* Called when an application closes a USB_UART */
static void usb_uart_shutdown(struct uart_port *port)
{
    /* ... */
    /* Free IRQ */
    free_irq(port->irq, port);

    /* Disable interrupts by writing to appropriate 
     * registers */
    /* ... */
}

/* Set UART type to USB_UART */
static const char * usb_uart_type(struct uart_port *port)
{
    return port->type == PORT_USB_UART ? "USB_UART" : NULL;
}

/* Start transmitting bytes */
static void usb_uart_start_tx(struct uart_port *port)
{
    while(1) {
        /* Get the data from the UART circular buffer and
         * write it to the USB_UART's WRITE_DATA register */
        usb_uart_putc(port,
                port->info->xmit.buf[port->info->xmit.tail]);
        /* Adjust the tail of the UART buffer */
        port->info->xmit.tail = (port->info->xmit.tail + 1) &
            (UART_XMIT_SIZE - 1);
        /* Statistics */
        port->icount.tx++;
        /* Finish if no more data available in the UART buffer */
        if (uart_circ_empty(&port->info->xmit)) break;
    }
    /* ... */
}

/* The UART operations structure */
static struct uart_opts usb_uart_ops = {
    .start_tx   =   usb_uart_start_tx,  /* Start transmitting */
    .startup   =   usb_uart_startup,  /* App opens USB_UART */
    .shutdown   =   usb_uart_shutdown,  /* App closes USB_UART */
    .type   =   usb_uart_type,  /* Set UART type */
    .config_port   =   usb_uart_config_port,  /* Configure when driver
                                                 adds a USB_UART port */
    .request_port   =   usb_uart_request_port,  /* Claim resources associated with a
                                                   USB_UART port */
    .release_port   =   usb_uart_release_port,  /* Release resources associated with a
                                                   USB_UART port */
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
    .cons  =   &usb_uart_console,    /* Pointer to the console
                                        structure. Discussed in Chapter
                                        12, "Video Drivers" */
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
