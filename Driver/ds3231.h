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
#include <asm/atomic.h>

#define true 1
#define false 0
#define null 0

#define LOCKED       0
#define UNLOCKED     1

/* Register Definitions */
#define DS3231_REG_SECONDS 0x00
#define DS3231_REG_MINUTES 0x01
#define DS3231_REG_HOURS 0x02
#define DS3231_REG_DAY 0x03
#define DS3231_REG_DATE 0x04
#define DS3231_REG_MONTH 0x05
#define DS3231_REG_YEAR 0x06
#define DS3231_REG_A1SECONDS 0x07
#define DS3231_REG_A1MINUTES 0x08
#define DS3231_REG_A1HOURS 0x09
#define DS3231_REG_A1DAY 0x0a
#define DS3231_REG_A2MINUTES 0x0b
#define DS3231_REG_A2HOURS 0x0c
#define DS3231_REG_A2DAY 0x0d
#define DS3231_REG_CONTROL 0x0e
#define DS3231_REG_STATUS 0x0f
#define DS3231_REG_AGEINGOFFSET 0x10
#define DS3231_REG_TEMPMSB 0x11
#define DS3231_REG_TEMPLSB 0x12

/* Register Mask Definitions */
#define DS3231_MASK_SECONDS 0b00001111u
#define DS3231_MASK_10_SECONDS 0b01110000u
#define DS3231_MASK_MINUTES 0b00001111u
#define DS3231_MASK_10_MINUTES 0b01110000u
#define DS3231_MASK_HOUR 0b00001111u
#define DS3231_MASK_10_HOUR 0b00010000u
#define DS3231_MASK_20_HOUR 0b00100000u
#define DS3231_MASK_HOUR_SELECT 0b01000000u
#define DS3231_MASK_DATE 0b00001111u
#define DS3231_MASK_10_DATE 0b00110000u
#define DS3231_MASK_MONTH 0b00001111u
#define DS3231_MASK_10_MONTH 0b00010000u
#define DS3231_MASK_CENTURY 0b10000000u
#define DS3231_MASK_YEAR 0b00001111u
#define DS3231_MASK_10_YEAR 0b11110000u

#define DS3231_MASK_EOSC 0b10000000u
#define DS3231_MASK_INTCN 0b00000100u
#define DS3231_MASK_A2IE 0b00000010u
#define DS3231_MASK_A1IE 0b00000001u
#define DS3231_MASK_OSF 0b10000000u
#define DS3231_MASK_BSY 0b00000100u

typedef struct _ds3231_time
{
    u8 second;
    u8 minute;
    u8 hour;

    u8 month;
    u8 day;
    u16 year;
} ds3231_time_t;

typedef struct _ds3231_status
{
    u8 rtc_busy;
    atomic_t drv_busy;
    u8 osf;
    s8 temp;
} ds3231_status_t;

extern ds3231_status_t ds3231_status;

int ds3231_hw_init(void);
void ds3231_hw_exit(void);

int ds3231_io_init(void);
void ds3231_io_exit(void);

int ds3231_hw_probe(struct i2c_client *client, const struct i2c_device_id *id);
int ds3231_hw_remove(struct i2c_client *client);

int ds3231_io_open(struct inode *inode, struct file *file);
int ds3231_io_close(struct inode *inode, struct file *file);
ssize_t ds3231_io_read(struct file *file, char __user *puffer, size_t bytes, loff_t *offset);
ssize_t ds3231_io_write(struct file *file, const char __user *puffer, size_t bytes, loff_t *offset);

/*****************************
 *       Internal API        *
 *****************************/

int ds3231_write_time(ds3231_time_t *time);
int ds3231_read_time(ds3231_time_t *time);

int ds3231_read_status(void);


/**
 * 
 * make[1]: Entering directory '/usr/src/linux-source-rpi2'
  CC [M]  /home/sysprog-02/ds3231_mod.o
  CC [M]  /home/sysprog-02/ds3231_hw.o
/home/sysprog-02/ds3231_hw.c: In function ‘ds3231_read_status’:
/home/sysprog-02/ds3231_hw.c:237:9: warning: ‘retval’ may be used uninitialized in this function [-Wmaybe-uninitialized]
     int retval;
         ^
  CC [M]  /home/sysprog-02/ds3231_io.o
/home/sysprog-02/ds3231_io.c: In function ‘ds3231_io_read’:
/home/sysprog-02/ds3231_io.c:141:5: warning: ISO C90 forbids mixed declarations and code [-Wdeclaration-after-statement]
     int bytes_to_copy = (bytes > sizeof(out) ? sizeof(out) : bytes);
     ^
/home/sysprog-02/ds3231_io.c: In function ‘ds3231_io_write’:
/home/sysprog-02/ds3231_io.c:165:5: warning: ignoring return value of ‘copy_from_user’, declared with attribute warn_unused_result [-Wunused-result]
     copy_from_user(in, buffer, bytes);
     ^
  LD [M]  /home/sysprog-02/ds3231_drv.o
  Building modules, stage 2.
  MODPOST 1 modules
  CC      /home/sysprog-02/ds3231_drv.mod.o
  LD [M]  /home/sysprog-02/ds3231_drv.ko
make[1]: Leaving directory '/usr/src/linux-source-rpi2'
sudo rmmod ds3231_drv
rmmod: ERROR: Module ds3231_drv is not currently loaded
Makefile:22: recipe for target 'reload' failed
make: [reload] Error 1 (ignored)
sudo insmod ds3231_drv.ko

 * 
 */