// Defined in include/linux/tty.h
struct tty_struct {
    int magic;                      /* Magic marker */
    struct tty_driver *driver;      /* Pointer to the tty driver */
    struct tty_ldisc ldisc;         /* Attached Line discipline */
    /* ... */
    struct tty_flip_buffer flip;    /* Flip Buffer. See below. */
    /* ... */

    wait_queue_head_t write_wait;   /* See the section "Line Disciplines" */
    wait_queue_head_t read_wait;   /* See the section "Line Disciplines" */
    /* ... */
};


// flip buffer embedded inside tty_struct. Centerpiece of data collection 
// and processing mechanism
struct tty_flip_buffer {
    /* ... */
    struct semaphore pty_sem;   /* Serialize */
    char *char_buf_ptr;         /* Pointer to the flip buffer */
    /* ... */
    unsigned char char_buf[2*TTY_FLIPBUF_SIZE]; /* The flip buffer */
    /* ... */
};

// in lateset kernel, tty_flip_buffer has some changes, it is currently 
// composed by tty_bufhead (buffer header) and tty_buffer(buffer list)
struct  tty_bufhead {
    /* ... */
    struct semaphore pty_sem;       /* Serialize */
    struct tty_buffer *head, tail, free; /* See below */
    /* ... */
};

struct tty_buffer {
    struct tty_buffer *next;    /* Pointer to the flip buffer */
    char *char_buf_ptr;
    /* ... */
    unsigned long data[0];      /* The flip buffer, memory for which is 
                                   dynamically allocated */
};


// Defined in include/linux/tty_driver.h
// It specified the interfaces between tty driver and upper layer
struct tty_driver {
    int magic;      /* Magic number */
    /* ... */
    int major;      /* Major number */
    int minor_start;    /* Start of minor number */
    /* ... */
    /* Interface routine between a tty driver and higher layers */
    int (*open)(struct tty_struct *tty, struct file *filp);
    void (*close)(struct tty_struct *tty, struct file *filp);
    int (*write)(struct tty_struct *tty, const unsigned char *buf, int count);
    void (*put_char)(struct tty_struct *tty, unsigned char ch);
    /* ... */
};


