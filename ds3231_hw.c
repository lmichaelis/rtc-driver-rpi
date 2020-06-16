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

/* Register Definitions */
#define DS3231_REG_SECONDS  	0x00
#define DS3231_REG_MINUTES		0x01
#define DS3231_REG_HOURS		0x02
#define DS3231_REG_DAY   		0x03
#define DS3231_REG_DATE 		0x04
#define DS3231_REG_MONTH		0x05
#define DS3231_REG_YEAR 		0x06
#define DS3231_REG_A1SECONDS	0x07
#define DS3231_REG_A1MINUTES	0x08
#define DS3231_REG_A1HOURS		0x09
#define DS3231_REG_A1DAY		0x0a
#define DS3231_REG_A2MINUTES	0x0b
#define DS3231_REG_A2HOURS		0x0c
#define DS3231_REG_A2DAY		0x0d
#define DS3231_REG_CONTROL		0x0e
#define DS3231_REG_STATUS		0x0f
#define DS3231_REG_AGEINGOFFSET	0x10
#define DS3231_REG_TEMPMSB		0x11
#define DS3231_REG_TEMPLSB		0x12

/* Singletons Definitions */
#define true 1
#define false 0
#define null 0

/* Register Mask Definitions */
#define DS3231_MASK_SECONDS    0b00001111
#define DS3231_MASK_10_SECONDS 0b01110000
#define DS3231_MASK_MINUTES    0b00001111
#define DS3231_MASK_10_MINUTES 0b01110000
#define DS3231_MASK_HOUR       0b00001111
#define DS3231_MASK_10_HOUR    0b00010000
#define DS3231_MASK_20_HOUR    0b00100000
#define DS3231_MASK_DATE       0b00001111
#define DS3231_MASK_10_DATE    0b00110000
#define DS3231_MASK_MONTH      0b00001111
#define DS3231_MASK_10_MONTH   0b00010000
#define DS3231_MASK_CENTURY    0b10000000
#define DS3231_MASK_YEAR       0b00001111
#define DS3231_MASK_10_YEAR    0b11110000

#define DS3231_MASK_EOSC       0b10000000
#define DS3231_MASK_INTCN      0b00000100
#define DS3231_MASK_A2IE       0b00000010
#define DS3231_MASK_A1IE       0b00000001
#define DS3231_MASK_OSF        0b10000000
#define DS3231_MASK_OSF        0b00000100

/****************************************************
 * (*) ds3231_hw.c  :: Hardware interfacing         *
 * ( ) ds3231_io.c  :: Character device interfacing *
 * ( ) ds3231_drv.c :: Linux module handling        *
 ****************************************************/
#include "ds3231.h"

static struct i2c_client *ds3231_client;

static const struct i2c_device_id ds3231_id[] = {
        {"ds3231_drv", 0},
        {}
};
MODULE_DEVICE_TABLE(i2c, ds3231_id);

static struct i2c_driver ds3231_hw_driver = {
        .driver = {
                .owner = THIS_MODULE,
                .name  = "ds3231_drv",
        },
        .id_table = ds3231_id,
        .probe	  = ds3231_probe,
        .remove	  = ds3231_remove,
};

int ds3231_hw_init(void) {
    const struct i2c_board_info info = {
            I2C_BOARD_INFO("ds3231_drv", 0x68)
    };

    ds3231_client = NULL;
    adapter = i2c_get_adapter(1);
    if(adapter == NULL) {
        printk("DS3231_drv: I2C Adapter nicht gefunden\n");
        return -ENODEV;
    }

    /* Neues I2C Device registrieren */
    ds3231_client = i2c_new_device(adapter, &info);
    if(ds3231_client == NULL) {
        printk("DS3231_drv: I2C Client: Registrierung fehlgeschlagen\n");
        return -ENODEV;
    }

    /* Treiber registrieren */
    ret = i2c_add_driver(&ds3231_driver);
    if(ret < 0) {
        printk("DS3231_drv: Treiber konnte nicht hinzugefügt werden (errorn = %d)\n", ret);
        i2c_unregister_device(ds3231_client);
        ds3231_client = NULL;
    }
    return ret;
}

int ds3231_hw_exit(void) {
}