/* Private struct used to impolement the Finite State Machine
 * (FSM) for the touch controller. The controller and the processor
 * communicate using a specific protocol that the FSM implements */
struct n_touch {
    int current_state;  /* Finite State Machine */
    spinlock_t touch_lock;  /* Spinlock */
    struct tty_struct *tty;  /* Associated tty */
    /* Stataistics and other housekeeping */
    /* ... */
} *n_tch;

/* Device open() */
static int n_touch_open(struct tty_struct *tty)
{
    /* Allocate memory for n_tch */
    if (!(n_tch = kmalloc(sizeof(struct n_touch), GFP_KERNEL))) {
        return -ENOMEM;
    }
    memset(n_tch, 0, sizeof(struct n_touch));

    tty->disc_data = n_tch; /* Other entry points now have direct
                               access to n_tch */
    /* Allocate the line discipline's local read buffer
     * used for copying data out of the tty flip buffer */
    tty->read_buf = kmalloc(BUFFER_SIZE, GFP_KERNEL);
    if (!tty->read_buf) return -ENOMEM;

    /* Clear the read buffer */
    memset(tty->read_buf, 0, BUFFER_SIZE);

    /* Initialize lock */
    spin_lock_init(&n_tch->touch_lock);

    /* Initialize other necessary tty fields.
     * See drivers/char/n_tty.c for an example */
    /* ... */

    return 0;
}


static void n_touch_receive_buf(struct tty_struct *tty, const unsigned char *cp,
        char *fp, int count)
{
    /* Work on the data in the line discipline's half of
     * the flip buffer pointed to by cp */
    /* ... */

    /* Implement the Finite State Machine to interpret commands/data
     * arriving from the touch controller and put the processed data
     * into the local read buffer */

    /* Datasheet-dependent Code Region */
    switch (tty->disc_data->current_state) {
        case RESET:
            /* Issue a reset command to the controller */
            tty->driver->write(tty, 0, mode_stream_command,
                    sizeof(mode_stream_command));
            tty->disc_data->current_state = STREAM_DATA;
            /* ... */
            break;
        case STREAM_DATA:
            /* ... */
            break;
        case PARSING:
            /* ... */
            tty->disc_data->current_state = PARSED;
            break;
        case PARSED:
            /* ... */
    }

    if (tty->disc_data->current_state == PARSED) {
        /* If you have a parsed packet, copy the collected coordinate
         * and direction information into the local read buffer */
        spin_lock_irqsave(&tty->disc_data->touch_lock, flag);
        for (i=0; i<PACKET_SIZE; i++) {
            tty->disc_data->read_buf[tty->disc_data->read_head] =
                tty->disc_data->current_pkt[i];
            tty->disc_data->read_head =
                (tty->disc_data->read_head + 1) & (BUFFER_SIZE - 1);
            tty->disc_data->read_cnt++;
        }
        spin_lock_irqrestore(&tty->disc_data->touch_lock, flags);
        /* ... */ 
        /* Wake up any threads waiting for data */
        if (waitqueue_active(&tty->read_wait) && 
                (tty->read_cnt >= tty->minimum_to_wait)) {
            wake_up_interruptible(&tty->read_wait);
        }
        /* If we are running out of buffer space, request the
         * serial driver to throttle incoming data */
        if (n_touch_receive_room(tty) < TOUCH_THROTTLE_THRESHOLD) {
            tty->driver.throttle(tty);
        }
        /* ... */
    }
} 

struct tty_ldisc n_touch_ldisc = {
    TTY_LDISC_MAGIC,        /* Magic */
    "n_tch",        /* Name of the line discipline */
    N_TCH,        /* Line discipline ID number */
    n_touch_open,        /* Open the line discipline */
    n_touch_close,        /* Close the line discipline */
    n_touch_flush_buffer, /* Flush the line discipline's 
                             read buffer */
    n_touch_chars_in_buffer, /* Get the number of processed characters
                                in the line discipline's read buffer */
    n_touch_read,            /* Called when data is requested
                                from user space */
    n_touch_write,            /* Write method */
    n_touch_ioctl,            /* I/O control commands */
    NULL,            /* We don't have a set_termios routine */
    n_touch_poll,    /* Poll */
    n_touch_receive_buf,    /* Called by the low-level driver
                               to pass data to user space */
    n_touch_receive_room,   /* Returns the room left in the line
                               discipline's read buffer */
    n_touch_write_wakeup    /* Called when the low-level device
                               driver is ready to transmit more data */
};

/* ... */

if ((err = tty_register_ldisc(N_TCH, &n_touch_ldisc))) {
    return err;
}
