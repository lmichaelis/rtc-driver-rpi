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

ssize_t ds3231_io_read(struct file *file, char __user *buffer, size_t bytes, loff_t *offset)
{
    
}

ssize_t ds3231_io_write(struct file *file, const char __user *buffer, size_t bytes, loff_t *offset)
{
    /* YYYY-MM-DD hh:mm:ss */
    /**
     * Check length == 19
     * buffer[4] = '\0'
     * 
     * 
     */
    int res, rval;
    rval = kstrtoint(buffer + 0, 10, &res);

    if (rval < 0)
        return rval;

    kstrtoint(buffer + 5, 10, &res);
}