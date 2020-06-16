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

int ds3231_hw_init(void);
void ds3231_hw_exit(void);

int ds3231_hw_probe(struct i2c_client *client, const struct i2c_device_id *id);
int ds3231_hw_remove(struct i2c_client *client);