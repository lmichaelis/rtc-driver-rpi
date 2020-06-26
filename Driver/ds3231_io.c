/****************************************************
 * ( ) ds3231_hw.c  :: Hardware interfacing         *
 * (*) ds3231_io.c  :: Character device interfacing *
 * ( ) ds3231_mod.c :: Linux module handling        *
 ****************************************************/
#include "ds3231.h"

dev_t ds3231_dev;
struct cdev ds3231_cdev;
struct class *ds3231_device_class;
struct file_operations ds3231_fops = {
    .owner = THIS_MODULE,
    .llseek = no_llseek,
    .read = ds3231_io_read,
    .write = ds3231_io_write,
    .open = ds3231_io_open,
    .release = ds3231_io_close,
};

/** All month names in German (may be changed) */
static const char *MONTH_NAMES[] = {
    "Januar",
    "Februar",
    "Maerz",
    "April",
    "Mai",
    "Juni",
    "Juli",
    "August",
    "September",
    "Oktober",
    "November",
    "Dezember"
};

/** The number of days in each month (february has to be handled differently in leap years.) */
static const int MONTH_DAYS[] = {
    31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};

/**
 * Creates and registers the linux character driver for the ds3231 real-time-clock.
 * The driver will be created with the name <tt>ds3231_drv</tt> and will allocate
 * only <tt>1</tt> minor number for the character device.
 *
 * @return <tt>0</tt> on success and a kernel error code on failure.
 * @brief Creates the character device
 */
int ds3231_io_init(void) {
    int ret;

    printk("ds3231: creating character device driver ...\n");

    /* Allocate a range of character device numbers */
    ret = alloc_chrdev_region(&ds3231_dev, 0, 1, "ds3231_drv");
    if (ret < 0) {
        printk(KERN_ERR "ds3231: alloc_chrdev_region() failed\n");
        return ret;
    }

    /* Initialize the cdev structure and add the driver to the system */
    cdev_init(&ds3231_cdev, &ds3231_fops);
    ret = cdev_add(&ds3231_cdev, ds3231_dev, 1);
    if (ret < 0) {
        printk(KERN_ERR "ds3231: character device could not be registered");
        goto unreg_chrdev;
    }

    /* Create a character device class */
    ds3231_device_class = class_create(THIS_MODULE, "chardev");
    if (ds3231_device_class == NULL) {
        printk(KERN_ERR "ds3231: character device class could not be created\n");
        goto clenup_cdev;
    }

    /* Create the character device */
    if (device_create(ds3231_device_class, NULL, ds3231_dev, NULL, "ds3231_drv") == NULL) {
        printk(KERN_ERR "ds3231: character device could not be created\n");
        goto cleanup_chrdev_class;
    }

    /* ds3231_drv initialized sucsessfully */
    printk("ds3231: character device driver successfully created\n");
    return 0;



    /*
     * Uninit on failure
     */
cleanup_chrdev_class:
    class_destroy(ds3231_device_class);
clenup_cdev:
    cdev_del(&ds3231_cdev);
unreg_chrdev:
    unregister_chrdev_region(ds3231_dev, 1);
    return -EIO;
}

/**
 * Destroys the character device and class and unregisters the character device
 * driver from the system. Also unregisters the character device region. Always
 * succeeds.
 *
 * @brief Destroys and frees all data associated with the character device driver
 */
void ds3231_io_exit(void) {
    device_destroy(ds3231_device_class, ds3231_dev);
    class_destroy(ds3231_device_class);
    cdev_del(&ds3231_cdev);
    unregister_chrdev_region(ds3231_dev, 1);
    printk("ds3231: unloaded chacter device driver\n");
}

/* ========================================================================== *
 *                            Open/Close Handlers                             *
 * ========================================================================== */

/**
 * Does nothing except write a debug message to the kernel log, just here
 * for completeness.
 *
 * @param inode The linux VFS inode for file-system access
 * @param file The file handle to store specific data and provide information
 * on how that file was opened.
 * @return <tt>0</tt>. This function never fails
 */
int ds3231_io_open(struct inode *inode, struct file *file) {
    printk(KERN_DEBUG "ds3231: opened character device\n");
    return 0;
}

/**
 * Does nothing except write a debug message to the kernel log, just here
 * for completeness.
 *
 * @param inode The linux VFS inode for file-system access
 * @param file The file handle to store specific data and provide information
 * on how that file was opened.
 * @return <tt>0</tt>. This function never fails
 */
int ds3231_io_close(struct inode *inode, struct file *file) {
    printk(KERN_DEBUG "ds3231: closed character device\n");
    return 0;
}

/* ========================================================================== *
 *                         User file read event                               *
 * ========================================================================== */

/**
 *
 * @param file
 * @param buffer
 * @param bytes
 * @param offset
 * @return
 */
ssize_t ds3231_io_read(struct file * file, char __user *buffer, size_t bytes, loff_t *offset)
{
    ds3231_time_t time;
    char out[28];
    int bytes_to_copy = (bytes > sizeof(out) ? sizeof(out) : bytes);
    int retval;

    /* Make sure the I2C bus is not in use. */
    if (atomic_cmpxchg(&ds3231_status.drv_busy, UNLOCKED, LOCKED) == LOCKED) {
        printk(KERN_ERR "ds3231: device busy\n");
        return -EBUSY;
    }

    /* Read the status register of the RTC */
    retval = ds3231_read_status();
    if (retval < 0) {
        atomic_set(&ds3231_status.drv_busy, UNLOCKED);
        return retval;
    }

    /* Read the time from the RTC */
    ds3231_read_time(&time);

    /* Bring the time into the correct format */
    memset(out, '\0', sizeof(out));
    snprintf(out, sizeof(out) - 1, "%02d. %s %02d:%02d:%02d %04d", time.day, MONTH_NAMES[time.month - 1], time.hour, time.minute, time.second, time.year);
    printk("ds3231: read time: %.*s\n", sizeof(out), out);

    atomic_set(&ds3231_status.drv_busy, UNLOCKED);
    return bytes_to_copy - copy_to_user(buffer, out, bytes_to_copy);
}

/* ========================================================================== *
 *                         User file write event                              *
 * ========================================================================== */

/**
 *
 * @param file
 * @param __user
 * @return
 */
ssize_t ds3231_io_write(struct file *file, const char __user *buffer, size_t bytes, loff_t *offset)
{
    ds3231_time_t time;
    s32 year, month, day, hour, minute, second, temp;
    int retval = 0;
    char in[bytes];

    /* Make sure the I2C bus is not in use.  */
    if (atomic_cmpxchg(&ds3231_status.drv_busy, UNLOCKED, LOCKED) == LOCKED) {
        printk(KERN_ERR "ds3231: device busy\n");
        return -EBUSY;
    }

    /* Get data from the user */
    (void) copy_from_user(in, buffer, bytes);

    if (in[0] == '$') {
        if (sscanf(in + 1, "%d", &temp) < 0) {
            atomic_set(&ds3231_status.drv_busy, UNLOCKED);
            return -ENOEXEC;
        }

        printk(KERN_ERR "ds3231: manual temperature override: %d°C\n", temp);
        ds3231_status.temp = (s8) temp;
        atomic_set(&ds3231_status.drv_busy, UNLOCKED);
        return bytes;
    }

    /* Read the status register of the RTC */
    retval = ds3231_read_status();
    if (retval < 0) {
        atomic_set(&ds3231_status.drv_busy, UNLOCKED);
        return retval;
    }

    /* Parse a date */
    if (sscanf(in, "%d-%d-%d %d:%d:%d", 
                &year, 
                &month, 
                &day, 
                &hour, 
                &minute, 
                &second) < 0) {
        atomic_set(&ds3231_status.drv_busy, UNLOCKED);
        return -EINVAL;
    }

    /* Make sure month, day of the month, hour, minute and second are in range */
    if ((12 < month || 1 > month) || 
        (MONTH_DAYS[month - 1] + (month == 2 ? (year % 4 == 0): 0) < day) || 
        (23 < hour || 0 > hour) || 
        (59 < minute || 0 > minute) ||
        (59 < second || 0 > second))
    {
        atomic_set(&ds3231_status.drv_busy, UNLOCKED);
        return -ENOEXEC;
    }

    /* Make sure the year is in range */
    if (2199 < year || 2000 > year) {
        atomic_set(&ds3231_status.drv_busy, UNLOCKED);
        return -EOVERFLOW;
    }

    time.year = year;
    time.month = month;
    time.day = day;
    time.hour = hour;
    time.minute = minute;
    time.second = second;

    printk("ds3231: write time %d. %d. %d %d:%d:%d\n", time.day, time.month, time.year, time.hour, time.minute, time.second);
    
    /* Write the time to the RTC */
    retval = ds3231_write_time(&time);
    atomic_set(&ds3231_status.drv_busy, UNLOCKED);
    return retval < 0 ? retval : bytes;
}