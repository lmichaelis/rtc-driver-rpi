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

int ds3231_io_init(void)
{
    int ret;

    printk("ds3231: creating character device driver ...\n");

    ret = alloc_chrdev_region(&ds3231_dev, 0, 1, "ds3231_drv");
    if (ret < 0)
    {
        printk(KERN_ALERT "ds3231: alloc_chrdev_region() failed\n");
        return ret;
    }

    cdev_init(&ds3231_cdev, &ds3231_fops);
    ret = cdev_add(&ds3231_cdev, ds3231_dev, 1);
    if (ret < 0)
    {
        printk(KERN_ALERT "ds3231: character device could not be registered");
        goto unreg_chrdev;
    }

    ds3231_device_class = class_create(THIS_MODULE, "chardev");
    if (ds3231_device_class == NULL)
    {
        printk(KERN_ALERT "ds3231: character device class could not be created\n");
        goto clenup_cdev;
    }

    if (device_create(ds3231_device_class, NULL, ds3231_dev, NULL, "ds3231_drv") == NULL)
    {
        printk(KERN_ALERT "ds3231: character device could not be created\n");
        goto cleanup_chrdev_class;
    }
    /* ds3231_drv initialized sucsessfully */
    printk("ds3231: character device driver sucsessfully created\n");

    return 0;

/* Resourcen freigeben und Fehler melden. */
cleanup_chrdev_class:
    class_destroy(ds3231_device_class);
clenup_cdev:
    cdev_del(&ds3231_cdev);
unreg_chrdev:
    unregister_chrdev_region(ds3231_dev, 1);
    return -EIO;
}

void ds3231_io_exit(void)
{
    device_destroy(ds3231_device_class, ds3231_dev);
    class_destroy(ds3231_device_class);
    cdev_del(&ds3231_cdev);
    unregister_chrdev_region(ds3231_dev, 1);
    printk("ds3231: unloaded chacter device driver\n");
}

/**********************************************
 *           Kernel Event Handling            *
 **********************************************/

int ds3231_io_open(struct inode *inode, struct file *file)
{
    return 0;
}

int ds3231_io_close(struct inode *inode, struct file *file)
{
    return 0;
}

/*

Q: Welche Kernel-Funktionen?
A: All

Q: BSY handling?
A: None

Q: Offset Stuff
A: None

------------------------------
Things to keep in mind:

- Leap year compensation only up to 2100
- Leap year checking in input

*/

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
    "Dezember" };

static const int MONTH_DAYS[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

ssize_t ds3231_io_read(struct file * file, char __user *buffer, size_t bytes, loff_t *offset)
{
    ds3231_time_t time;
    ds3231_read_time(&time);
    char out[4 /* "DD. " */ + 10 /* "M " */ + 13 /* "hh:mm:ss YYYY" */ + 1 /* \0 */];
    int bytes_to_copy = bytes > sizeof(out) ? sizeof(out) : bytes;
    out[sizeof(out) - 1] = '\0';

    snprintf(out, sizeof(out), "%2d. %s %2d:%2d:%2d %4d", time.day, MONTH_NAMES[time.month - 1], time.hour, time.minute, time.second, time.year);
    printk("ds3231: read time: %.*s\n", sizeof(out), out);
    
    return copy_to_user(buffer, out, bytes_to_copy);
}

ssize_t ds3231_io_write(struct file *file, const char __user *buffer, size_t bytes, loff_t *offset)
{
    ds3231_time_t time;
    s32 year, month, day, hour, minute, second;
    char in[bytes];

    copy_from_user(in, buffer, bytes);


    if (sscanf(in, "%d-%d-%d %d:%d:%d", 
                &year, 
                &month, 
                &day, 
                &hour, 
                &minute, 
                &second) < 0) {
        return -EINVAL;
    }

    if ((12 < month || 1 > month) || 
        (MONTH_DAYS[month - 1] + (month == 2 ? (year % 4 == 0): 0) < day) || 
        (23 < hour || 0 > hour) || 
        (59 < minute || 0 > minute) ||
        (59 < second || 0 > second))
    {
        return -ENOEXEC;
    }

    if (!(2199 < year || 2000 > year)) {
        return -EOVERFLOW;
    }

    time.year = year;
    time.month = month;
    time.day = day;
    time.hour = hour;
    time.minute = minute;
    time.second = second;

    ds3231_write_time(&time);
    return 0;
}