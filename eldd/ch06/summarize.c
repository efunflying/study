// buttom layer:  uart driver
// Main skeleton for init: 
// 1. uart_register_driver (Register the USB_UART driver with the serial core)
// 2. platform_device_register_simple (Register platform device for USB_UART)
// 3. platform_driver_register (Announce a matching driver for the platform devices registered above)
// Main skeleton for exit is reversed with init.


/*  struct uart_driver, defined in include/linux/serial_core.h
struct uart_driver {
    struct module *owner;   // Module that owns this struct 
    const char *driver_name;   // Name 
    const char *dev_name;   // /dev node name such as ttyS
    //...
    int major;  // Major number
    int minor;  // Minor number
    //...
    struct tty_driver *tty_driver;  // tty driver
};
*/

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


/*  struct uart_port, defined in include/linux/serial_core.h
 *
struct uart_port {
    spinlock_t lock;   // port lock 
    unsigned int iobase;   // in/out[bwl] 
    unsigned char __iomem *membase; // read/write[bwl]
    unsigned int irq;   // irq number
    unsigned int uartclk;   // base uart clock
    unsigned char fifosize; // tx fifo size
    unsigned char x_char;   // xon/xoff flow control
    //...
};
*/

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

/*  struct uart_ops, defined in include/linux/serial_core.h
 *
struct uart_ops {
    uint (*tx_empty)(struct uart_port *);   // Is TX FIFO empty?
    void (*set_mctrl)(struct uart_port *,
                      unsigned int mctrl);   // Set modem control params
    uint (*get_mctrl)(struct uart_port *);  // Get modem control params
    void (*stop_tx)(struct uart_port *);  // Stop xmission
    void (*start_tx)(struct uart_port *);  // Start xmission

    //...
    void (*shutdown)(struct uart_port *);   // Disable the port
    void (*set_termios)(struct uart_port *,
                        struct termios *new,
                        struct termios *old);   // Set terminal interface params

    //...
    void (*config_port)(struct uart_port *, int);   // Configure UART port
    //...
};
*/

/* The UART operations structure */
static struct uart_ops usb_uart_ops = {
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

// Middle Layer: tty driver

/* Like a UART driver, a tty driver needs to perform
 * two steps to register itself with the kernel: 
 * 1. Call tty_register_driver(struct tty_driver *tty_d) to register itself with the tty core.
 * 2. Call tty_register_device(struct tty_driver *tty_d,
 *                             unsigned device_index,
 *                             struct device *device)
 *    to register each individual tty that it supports
 * */

/* Related core data structures 

// struct tty_struct defined in include/linux/tty.h. This structure contains all state information
// associated with an open tty. Here're some important field.
struct tty_struct {
    int magic;  // Magic marker
    struct tty_driver *driver;  // Pointer to the tty driver
    struct tty_ldisc ldisc;  // Attached Line discipline
    //...
    struct tty_flip_buffer flip;    // Flip buffer. See below.
    //...
    wait_queue_head_t write_wait;   // See the section "Line Disciplines"
    wait_queue_head_t read_wait;    // See the section "Line Disciplines"
    //...
}

// struct tty_flip_buffer or the flilp buffer embedded inside tty_struct. It is the 
// centerpiece of the data collection and processing mechanism. The low-level serial driver
// uses one half of the flip buffer for gathering data, while the line discipline uses the
// other half for processing the data. The buffer pointers used bye the serial driver and the 
// line discipline are then flipped, and the process continues. flush_do_ldisc() in 
// drivers/char/tty_io.c demos such a flipping action.
struct tty_flip_buffer {
};
=>
struct tty_bufhead {
    //...
    struct semaphore pty_sem;   // Serialize
    struct tty_buffer *head, tail, free;    // See below
};
+
struct tty_buffer {
    struct tty_buffer *next;
    char *char_buf_ptr; // Pointer to the flip buffer
    //...
    //unsigned long data[0];    // The flip buffer, memory for
                                // which is dynamically
                                // allocated
};

// struct tty_driver defined in include/linux/tty_driver.h. This
// specifies the programming interface between tty drivers and 
// higher layers
struct tty_driver {
    int magic;  // Magic number
    //...
    int major;  // Major number
    int minor_start;    // Start of minor number
    //...
    // Interface routines between a tty driver and higher layers
    int (*open)(struct tty_struct *tty, struct file *filp);
    void (*close)(struct tty_struct *tty, struct file *filp);
    int (*write)(struct tty_struct *tty,
                 const unsigned char *buf, int count);
    void (*put_char)(struct tty_struct *tty, unsigned char ch);
    //...
};

 */


