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

/* Singletons Definitions */
#define true 1
#define false 0
#define null 0

/****************************************************
 * ( ) ds3231_hw.c  :: Hardware interfacing         *
 * (*) ds3231_io.c  :: Character device interfacing *
 * ( ) ds3231_drv.c :: Linux module handling        *
 ****************************************************/
#include "ds3231.h"

/* Driver Identifier: ds3231_drv  */

void ds3231_io_init(void) {
}

void ds3231_io_exit(void) {
}