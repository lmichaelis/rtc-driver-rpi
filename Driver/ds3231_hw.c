/****************************************************
 * (*) ds3231_hw.c  :: Hardware interfacing         *
 * ( ) ds3231_io.c  :: Character device interfacing *
 * ( ) ds3231_mod.c :: Linux module handling        *
 ****************************************************/
#include "ds3231.h"

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
#define DS3231_MASK_SECONDS     0b00001111
#define DS3231_MASK_10_SECONDS  0b01110000
#define DS3231_MASK_MINUTES     0b00001111
#define DS3231_MASK_10_MINUTES  0b01110000
#define DS3231_MASK_HOUR        0b00001111
#define DS3231_MASK_10_HOUR     0b00010000
#define DS3231_MASK_20_HOUR     0b00100000
#define DS3231_MASK_HOUR_SELECT 0b01000000
#define DS3231_MASK_DATE        0b00001111
#define DS3231_MASK_10_DATE     0b00110000
#define DS3231_MASK_MONTH       0b00001111
#define DS3231_MASK_10_MONTH    0b00010000
#define DS3231_MASK_CENTURY     0b10000000
#define DS3231_MASK_YEAR        0b00001111
#define DS3231_MASK_10_YEAR     0b11110000

#define DS3231_MASK_EOSC       0b10000000
#define DS3231_MASK_INTCN      0b00000100
#define DS3231_MASK_A2IE       0b00000010
#define DS3231_MASK_A1IE       0b00000001
#define DS3231_MASK_OSF        0b10000000
#define DS3231_MASK_BSY        0b00000100

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
        .probe = ds3231_hw_probe,
        .remove = ds3231_hw_remove,
};

/**********************************************
 *              Init / Exit                   *
 **********************************************/

int ds3231_hw_init(void) {
    const struct i2c_board_info info = { I2C_BOARD_INFO("ds3231_drv", 0x68) };
    struct i2c_adapter *adapter;
    int rval;

    printk("ds3231: initializing hardware ...\n");

    ds3231_client = null;
    adapter = i2c_get_adapter(1);
    if (adapter == null) {
        printk(KERN_ERR "ds3231: i2c adapter not found\n");
        return -ENODEV;
    }

    ds3231_client = i2c_new_device(adapter, &info);
    if(ds3231_client == null) {
        printk(KERN_ERR "ds3231: failed to register i2c device\n");
        return -ENODEV;
    }

    rval = i2c_add_driver(&ds3231_hw_driver);
    if(rval < 0) {
        printk(KERN_ERR "ds3231: failed to ds3231 i2c driver\n");
        i2c_unregister_device(ds3231_client);
        ds3231_client = null;
    }

    printk("ds3231: hardware initialization completed.\n");
    return rval;
}

void ds3231_hw_exit(void) {
    printk("ds3231: uninitializing hardware ...\n");
    if(ds3231_client != null) {
        i2c_del_driver(&ds3231_hw_driver);
        i2c_unregister_device(ds3231_client);
        ds3231_client = null;
    }
}

/**********************************************
 *           Kernel Event Handling            *
 **********************************************/

int ds3231_hw_probe(struct i2c_client *client, const struct i2c_device_id *id) {
    s32 data;
    u8 reg;

    printk("ds3231: setting up RTC ...\n");

    /*
     * Disable Alarm 1, Alarm 2 and interrupts. Enable oscillator
     */
    data = i2c_smbus_read_byte_data(client, DS3231_REG_CONTROL);
    if (data < 0) {
        // Failed to read control register
    }

    reg = (u8) data;
    if (reg & DS3231_MASK_A1IE || reg & DS3231_MASK_A2IE || reg & DS3231_MASK_INTCN || reg & DS3231_MASK_EOSC) {
        reg &= ~DS3231_MASK_A1IE;
        reg &= ~DS3231_MASK_A2IE;
        reg &= ~DS3231_MASK_INTCN;
        reg &= ~DS3231_MASK_EOSC;
        i2c_smbus_write_byte_data(client, DS3231_REG_CONTROL, reg);
        printk("ds3231: disabled alarm1, alarm2, interrupts; enabled oscillator.\n");
    }
    /*
     * Check oscillator stop flag
     */

    data = i2c_smbus_read_byte_data(client, DS3231_REG_STATUS);
    if (data < 0) {
        // Failed to read status register
    }

    reg = (u8) data;
    if (reg & DS3231_MASK_OSF) {
        reg &= ~DS3231_MASK_OSF;
        i2c_smbus_write_byte_data(client, DS3231_REG_STATUS, reg);
        printk("ds3231: reset oscillator stop flag (oscillator was stopped).\n");
    }

    /*
     * Set the RTC to 24 hr mode
     */

    data = i2c_smbus_read_byte_data(client, DS3231_REG_HOURS);
    if (data < 0) {
        // Failed to read status register
    }

    reg = (u8) data;
    if (reg & DS3231_MASK_HOUR_SELECT) {
        reg &= ~DS3231_MASK_HOUR_SELECT;
        i2c_smbus_write_byte_data(client, DS3231_REG_HOURS, reg);
        printk("ds3231: set to 24 hour format.\n");
    }

    return 0;
}

int ds3231_hw_remove(struct i2c_client *client) {
    printk("ds3231: ds3231_remove called\n");
    return 0;
}
