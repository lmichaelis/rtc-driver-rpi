#include <linux/slab.h>
#include <linux/bcd.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/i2c.h>
#include <linux/bcd.h>
#include <linux/rtc.h>
#include <linux/interrupt.h>
#include <linux/uaccess.h>
#include <asm/errno.h>
#include <asm/delay.h>

/****************************************************
 * ( ) ds3231_hw.c  :: Hardware interfacing         *
 * ( ) ds3231_io.c  :: Character device interfacing *
 * (*) ds3231_mod.c :: Linux module handling        *
 ****************************************************/
#include "ds3231.h"

ds3231_status_t ds3231_status;

/**
 * Registers the driver with the linux kernel and sets up the RTC.
 * It first sets up the RTC hardware and then registers the linux
 * character device.
 *
 * See <tt>ds3231_hw_init(void)</tt> and <tt>ds3231_io_init(void)</tt>
 * for more information about the setup procedure.
 *
 * @see ds3231_hw_init(void)
 * @see ds3231_io_init(void)
 *
 * @brief Initializes the ds3231 RTC driver.
 */
static int __init ds3231_drv_init(void) {
    int rval = ds3231_hw_init();
    if (rval < 0) {
        return rval;
    }

    rval = ds3231_io_init();
    if (rval < 0) {
      ds3231_hw_exit();
    }

    atomic_set(&ds3231_status.drv_busy, UNLOCKED);
    return rval;
}

/**
 * Unregisters the driver from the linux kernel. First it unregisters the
 * character device, then removes the I2C driver and associated device.
 *
 * See <tt>ds3231_hw_exit(void)</tt> and <tt>ds3231_io_exit(void)</tt>
 * for more information about the unregister procedure.
 *
 * @see ds3231_hw_exit(void)
 * @see ds3231_io_exit(void)
 *
 * @brief Uninitializes the ds3231 RTC driver.
 */
static void __exit ds3231_drv_exit(void) {
    ds3231_io_exit();
    ds3231_hw_exit();
}

module_init(ds3231_drv_init);
module_exit(ds3231_drv_exit);

MODULE_AUTHOR("Luis Michaelis <uk069904@student.uni-kassel.de>, Philip Laskowicz <philip.laskowicz@student.uni-kassel.de>");
MODULE_DESCRIPTION("DS3231 RTC Driver");
MODULE_LICENSE("GPL");