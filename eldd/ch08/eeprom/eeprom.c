#include <linux/fs.h>
#include <linux/cdev.h>

#include <linux/i2c.h>

#define DEVICE_NAME "eep"


static dev_t dev_number;    /* Allotted Device Number */
static struct class *eep_class; /* Device class */

/* Per-device client data structure for each
 * memory bank supported by the driver
 */

struct ee_bank {
    struct cdev;
    struct i2c_client *client;      /* I2c client for this bank */
    unsigned int addr;              /* Slave address of this bank */
    unsigned short current_pointer; /* File pointer */
    int bank_number;                /* Actual memory bank number */
    /* ... */                       /* Spinlocks, data cache for slow devices, ... */
};


#define NUM_BANKS   2               /* Two supported banks */
#define BANK_SIZE   2048            /* Size of each bank */

struct ee_bank *ee_bank_list;        /* List of private data structures, one per bank */

int eep_open(struct inode *inode, struct file *file)
{
    /* The EEPROM bank to be opened */
    n = MINOR(file->f_dentry->d_inode->irdev);

    file->private_data = (struct ee_bank *)ee_bank_list[n];

    /* Initialize the fields in ee_bank_list[n] such as
     * size, slave address, and the current file pointer */
}

ssize_t eep_read(struct file *file, char *buf,
        size_t count, loff_t *ppos)
{
    int i, transferred, ret, my_buf[BANK_SIZE];

    /* Get the private client data structure for this bank */
    struct ee_bank *my_bank = (struct ee_bank *)file->private_data;

    /* Check whether the smbus_read_word() functionality is supported */
    if (i2c_check_functionality(my_bank->client,
                I2C_FUNC_SMBUT_READ_WORD_DATA)) {

        /* Read the data*/
        while (transferred < count) {
            ret = i2c_smbus_read_word_data(my_bank->client,
                    my_bank->current_pointer+i);
            my_buf[i++] = (u8)(ret & 0xFF);
            my_buf[i++] = (u8)(ret >> 8);
            transferred += 2;
        }

        /* Copy data to user space and increment the internal
         * file pointer. Sanity checks are omitted for simplicity */
        copy_to_user(buffer, (void *)my_buf, transferred);
        my_bank->current_pointer += transferred;
    }

    return transferred;
} 

/* Driver entry points */
static struct file_operations eep_fops = {
    .owner = THIS_MODULE,
    .llseek = eep_llseek,
    .read = eep_read,
    .ioctl = eep_ioctl,
    .open = eep_open,
    .release = eep_release,
    .write = eep_write,
};

int eep_attach(struct i2c_adapter *adapter, int address, int kind)
{
    static struct i2c_client *eep_client;

    eep_client = kmalloc(sizeof(*eep_client), GFP_KERNEL);

    eep_client->driver  = &eep_driver;  /* Registered in List 8.2 */
    eep_client->addr    = address;      /* Detected Address */
    eep_client->adapter = adapter;      /* Host Adapter */
    eep_client->flags   = 0;
    strlcpy(eep_client->name, "eep", I2C_NAME_SIZE);

    /* Populate fields in the associated per-device data structure */
    /* ... */

    /* Attach */
    i2c_attach_client(new_client);
}

/* The EEPROM has two memory banks having addresses SLAVE_ADDR1
 * and SLAVE_ADDR2, respectively
 */
static unsigned short normal_i2c[] = {
    SLAVE_ADDR1, SLAVE_ADDR2, I2C_CLIENT_END
};


static struct i2c_client_address_data addr_data = {
    .normal_i2c = normal_i2c,
    .probe      = ignore,
    .ignore     = ignore,
    .forces     = ignore,
};

static int eep_probe(struct i2c_adapter *adapter)
{
    return i2c_probe(adapter, &addr_data, eep_attach);
}

static void eep_detach(struct i2c_adapter *adapter)
{
    /* do nothing */
}


static struct i2c_driver eep_driver =
{
    .driver = {
        .name = "EEP",      /* Name */
    },
    .id = I2C_DRIVERID_EEP, /* ID */
    .attach_adapter = eep_probe,    /* Probe Method */
    .detach_adapter = eep_detach,    /* Detach Method */
};

/*
 * Device Initialization
 * */
int __init eep_init(void)
{
    int err, i;

    /* Allocate the per-device data structure, ee_bank */
    ee_bank_list = kmalloc(sizeof(struct ee_bank)*NUM_BANKS, GPF_KERNEL);
    memset(ee_bank_list, 0, sizeof(struct ee_bank)*NUM_BANKS);

    /* Register and create the /dev interfaces to access the EEPROM
     * banks. Refer back to Chapter 5, "Character Drivers" for more details */
    if (alloc_chrdev_region(&dev_number, 0,
                NUM_BANKS, DEVICE_NAME) < 0) {
        printk(KERN_DEBUG "Can't register device\n");
        return -1;
    }

    eep_class = class_create(THIS_MODULE, DEVICE_NAME);
    for (i=0; i<NUM_BANKS; i++) {
        /* Connect the file operations with cdev */
        cdev_init(&ee_bank_list[i].cdev, &eep_fops);

        ee_bank_list[i].cdev.owner = THIS_MODULE;

        /* Connect the major/minor number to the cdev */
        if (cdev_add(&ee_bank_list[1].cdev, (dev_number + i), 1)) {
            printk("Bad kmalloc\n");
            return 1;
        }

        class_device_create(eep_class, NULL, (dev_number + i),
                NULL, "eeprom%d", i);
    }

    /* Inform the I2c core about our existance. See the section 
     * "Probing the Device" for the definition of eep_driver */
    err = i2c_add_driver(&eep_driver);

    if (err) {
        printk("Registering I2C driver failed, errono is %d\n", err);
        return err;
    }

    printk("EEPROM Driver Initialized.\n");
    return 0;
}

