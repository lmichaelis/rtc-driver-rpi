/****************************************************
 * (*) ds3231_hw.c  :: Hardware interfacing         *
 * ( ) ds3231_io.c  :: Character device interfacing *
 * ( ) ds3231_mod.c :: Linux module handling        *
 ****************************************************/
#include "ds3231.h"

/** The I2C client for interfacing with the DS3231 RTC */
static struct i2c_client *ds3231_client;

/** RTC device ID */
static const struct i2c_device_id ds3231_id[] = {
    {"ds3231_drv", 0},
    {}};
MODULE_DEVICE_TABLE(i2c, ds3231_id);

/** RTC driver configuration */
static struct i2c_driver ds3231_hw_driver = {
    .driver = {
        .owner = THIS_MODULE,
        .name = "ds3231_drv",
    },
    .id_table = ds3231_id,
    .probe = ds3231_hw_probe,
    .remove = ds3231_hw_remove,
};

int ds3231_hw_init(void)
{
    const struct i2c_board_info info = {I2C_BOARD_INFO("ds3231_drv", 0x68)};
    struct i2c_adapter *adapter;
    int rval;

    pr_info("ds3231: initializing hardware ...\n");

    /* Get the correct I2C adapter to use */
    ds3231_client = null;
    adapter = i2c_get_adapter(1);
    if (adapter == null) {
        pr_err("ds3231: i2c adapter not found\n");
        return -ENODEV;
    }

    /* Open the correct I2C device */
    ds3231_client = i2c_new_device(adapter, &info);
    if (ds3231_client == null) {
        pr_err("ds3231: failed to register i2c device\n");
        return -ENODEV;
    }

    /* Register this as a driver */
    rval = i2c_add_driver(&ds3231_hw_driver);
    if (rval < 0) {
        pr_err("ds3231: failed to ds3231 i2c driver\n");
        i2c_unregister_device(ds3231_client);
        ds3231_client = null;
    }

    pr_info("ds3231: hardware initialization completed.\n");
    return rval;
}


void ds3231_hw_exit(void)
{
    pr_info("ds3231: uninitializing hardware ...\n");
    if (ds3231_client != null)
    {
    	/* Delete the driver and unregister the device. */
        i2c_del_driver(&ds3231_hw_driver);
        i2c_unregister_device(ds3231_client);
        ds3231_client = null;
    }
}


int ds3231_hw_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    s32 data;
    u8 reg;

    pr_info("ds3231: setting up RTC ...\n");

    /* Disable Alarm 1, Alarm 2 and interrupts. Enable oscillator */
    data = i2c_smbus_read_byte_data(client, DS3231_REG_CONTROL);
    if (data < 0) {
        goto failed_to_comm;
    }

    reg = (u8)data;
    if (reg & DS3231_MASK_A1IE || reg & DS3231_MASK_A2IE || reg & DS3231_MASK_INTCN || reg & DS3231_MASK_EOSC)
    {
        reg &= ~DS3231_MASK_A1IE;
        reg &= ~DS3231_MASK_A2IE;
        reg &= ~DS3231_MASK_INTCN;
        reg &= ~DS3231_MASK_EOSC;

        if (i2c_smbus_write_byte_data(client, DS3231_REG_CONTROL, reg) < 0) {
            goto failed_to_comm;
        }

        pr_debug("ds3231: disabled alarm1, alarm2, interrupts; enabled oscillator.\n");
    }

    /* Check oscillator stop flag */
    data = i2c_smbus_read_byte_data(client, DS3231_REG_STATUS);
    if (data < 0) {
        goto failed_to_comm;
    }

    reg = (u8)data;
    if (reg & DS3231_MASK_OSF) {
        reg &= ~DS3231_MASK_OSF;

        if (i2c_smbus_write_byte_data(client, DS3231_REG_STATUS, reg) < 0) {
            goto failed_to_comm;
        }

        pr_debug("ds3231: reset oscillator stop flag (oscillator was stopped).\n");
    }

    /* Set the RTC to 24 hr mode */
    data = i2c_smbus_read_byte_data(client, DS3231_REG_HOURS);
    if (data < 0) {
        goto failed_to_comm;
    }

    reg = (u8)data;
    if (reg & DS3231_MASK_HOUR_SELECT) {
        reg &= ~DS3231_MASK_HOUR_SELECT;

        if (i2c_smbus_write_byte_data(client, DS3231_REG_HOURS, reg) < 0){
            goto failed_to_comm;
        }

        pr_debug("ds3231: set to 24 hour format.\n");
    }

    return 0;

failed_to_comm:
    pr_err("ds3231: device could not be communicated with\n");
    return -ENODEV;
}

int ds3231_hw_remove(struct i2c_client *client)
{
    pr_err("ds3231: ds3231_remove called\n");
    return 0;
}

#define RETURN_IF_LTZ(x, y) \
    y = x;                  \
    if (y < 0) {            \
        return y;           \
    }

int ds3231_write_time(ds3231_time_t *time)
{
    u8 secs, mins, hrs, date, mon, year;
    int retval;

	/* Convert to binary. See "Timekeeping Registers" on page 11 of the DS3231 manual.*/
    secs = ((time->second / 10) << 4) | (time->second % 10);
    mins = ((time->minute / 10) << 4) | (time->minute % 10);
    hrs = ((time->hour / 20) << 5) | (((time->hour % 20) / 10) << 4) | ((time->hour % 20) % 10);
    date = ((time->day / 10) << 4) | (time->day % 10);
    mon = ((time->year / 100) << 7) | ((time->month / 10) << 4) | ((time->month % 10));
    year = (((time->year % 100) / 10) << 4) | ((time->year % 100) % 10);

    /* Write to the RTC */
    RETURN_IF_LTZ(i2c_smbus_write_byte_data(ds3231_client, DS3231_REG_SECONDS, secs), retval);
    RETURN_IF_LTZ(i2c_smbus_write_byte_data(ds3231_client, DS3231_REG_MINUTES, mins), retval);
    RETURN_IF_LTZ(i2c_smbus_write_byte_data(ds3231_client, DS3231_REG_HOURS, hrs), retval);
    RETURN_IF_LTZ(i2c_smbus_write_byte_data(ds3231_client, DS3231_REG_DATE, date), retval);
    RETURN_IF_LTZ(i2c_smbus_write_byte_data(ds3231_client, DS3231_REG_MONTH, mon), retval);
    RETURN_IF_LTZ(i2c_smbus_write_byte_data(ds3231_client, DS3231_REG_YEAR, year), retval);

    return retval;
}


int ds3231_read_time(ds3231_time_t *time)
{
    s32 reg_secs, reg_mins, reg_hrs, reg_date, reg_mon, reg_year;
    u8 secs, mins, hrs, date, mon, year;

    /* Read from the RTC */
    RETURN_IF_LTZ(i2c_smbus_read_byte_data(ds3231_client, DS3231_REG_SECONDS), reg_secs);
    RETURN_IF_LTZ(i2c_smbus_read_byte_data(ds3231_client, DS3231_REG_MINUTES), reg_mins);
    RETURN_IF_LTZ(i2c_smbus_read_byte_data(ds3231_client, DS3231_REG_HOURS), reg_hrs);
    RETURN_IF_LTZ(i2c_smbus_read_byte_data(ds3231_client, DS3231_REG_DATE), reg_date);
    RETURN_IF_LTZ(i2c_smbus_read_byte_data(ds3231_client, DS3231_REG_MONTH), reg_mon);
    RETURN_IF_LTZ(i2c_smbus_read_byte_data(ds3231_client, DS3231_REG_YEAR), reg_year);

    secs = (u8)reg_secs;
    mins = (u8)reg_mins;
    hrs = (u8)reg_hrs;
    date = (u8)reg_date;
    mon = (u8)reg_mon;
    year = (u8)reg_year;

    /* Convert to decimal. See "Timekeeping Registers" on page 11 of the DS3231 manual.*/
    time->second = 10 * (secs >> 4) + (secs & DS3231_MASK_SECONDS);
    time->minute = 10 * (mins >> 4) + (mins & DS3231_MASK_MINUTES);
    time->hour = 20 * ((hrs >> 5) & 1) + 10 * ((hrs >> 4) & 1) + (hrs & DS3231_MASK_HOUR);
    time->day = 10 * (date >> 4) + (date & DS3231_MASK_DATE);
    time->month = 10 * ((mon >> 4) & 1) + (mon & DS3231_MASK_MONTH);
    time->year = 2000 + 100 * (mon >> 7) + 10 * (year >> 4) + (year & DS3231_MASK_YEAR);

    return 0;
}


int ds3231_read_status(void)
{
    s32 reg_status, reg_temp;
    u8 status, control;
    int retval = 0;

    RETURN_IF_LTZ(i2c_smbus_read_byte_data(ds3231_client, DS3231_REG_STATUS), reg_status);    
    status = (u8)reg_status;

    if (!ds3231_status.drv_temp_test) {
        RETURN_IF_LTZ(i2c_smbus_read_byte_data(ds3231_client, DS3231_REG_TEMPMSB), reg_temp);
        ds3231_status.temp = (s8)reg_temp;
    }

    ds3231_status.osf = (status >> 7);
    ds3231_status.rtc_busy = ((status & DS3231_MASK_BSY) << 1);

    if (ds3231_status.osf)
    {
        pr_notice("ds3231: oscillator stopped. restarting ...\n");
        control = i2c_smbus_read_byte_data(ds3231_client, DS3231_REG_CONTROL);
        RETURN_IF_LTZ(i2c_smbus_write_byte_data(ds3231_client, DS3231_REG_CONTROL, (control | DS3231_MASK_EOSC)), retval);
        RETURN_IF_LTZ(i2c_smbus_write_byte_data(ds3231_client, DS3231_REG_STATUS, (status & (~DS3231_MASK_OSF))), retval);
        return -EAGAIN;
    }

    if ((ds3231_status.temp > 85) || (ds3231_status.temp < -40))
    {
        pr_notice("ds3231: temperature warning: %d°C\n", ds3231_status.temp);
    }

    /** Reset temperature test flag */
    ds3231_status.drv_temp_test = 0;
    return retval;
}