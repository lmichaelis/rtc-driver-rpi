#include <linux/slab.h>
#include <linux/bcd.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/i2c.h>
#include <linux/rtc.h>
#include <linux/interrupt.h>
#include <linux/uaccess.h>
#include <asm/errno.h>
#include <asm/delay.h>
#include <asm/atomic.h>

#define null 0
#define LOCKED       0
#define UNLOCKED     1

/**
 * @defgroup Registers
 * Register addresses for the DS3231 real-time-clock.
 */

/**
 * @addtogroup Time
 * @ingroup Registers
 *
 * @{
 */
#define DS3231_REG_SECONDS 0x00
#define DS3231_REG_MINUTES 0x01
#define DS3231_REG_HOURS 0x02
#define DS3231_REG_DAY 0x03
#define DS3231_REG_DATE 0x04
#define DS3231_REG_MONTH 0x05
#define DS3231_REG_YEAR 0x06
/** @} */

/**
 * @addtogroup Alarm
 * @ingroup Registers
 *
 * @{
 */
#define DS3231_REG_A1SECONDS 0x07
#define DS3231_REG_A1MINUTES 0x08
#define DS3231_REG_A1HOURS 0x09
#define DS3231_REG_A1DAY 0x0a
#define DS3231_REG_A2MINUTES 0x0b
#define DS3231_REG_A2HOURS 0x0c
#define DS3231_REG_A2DAY 0x0d
/** @} */

/**
 * @addtogroup Control
 * @ingroup Registers
 *
 * @{
 */
#define DS3231_REG_CONTROL 0x0e
#define DS3231_REG_STATUS 0x0f
#define DS3231_REG_AGEINGOFFSET 0x10
#define DS3231_REG_TEMPMSB 0x11
#define DS3231_REG_TEMPLSB 0x12
/** @} */


/**
 * @addtogroup Register Value Bitmasks
 * Needed for the correct extraction of time data from the DS3231 real-time-clock.
 * See <tt>Timekeeping Registers</tt> on page 11 of the DS3231 manual.
 *
 * @{
 */
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
/** @} */

typedef struct _ds3231_time
{
    u8 second; /**< Second part of the time (0-59) */
    u8 minute; /**< Minute part of the time (0-59) */
    u8 hour; /**< Hour part of the time (0-23) */

    u8 month;  /**< Month part of the time (1-12) */
    u8 day;  /**< Day of the month part of the time (0-32) */
    u16 year;  /**< Year part of the time (2000-2199) */
} ds3231_time_t;

typedef struct _ds3231_status
{
    u8 rtc_busy; /**< Busy flag of the real-time-clock chip (updated on every read/write operation) */
    atomic_t drv_busy; /**< Busy flag of the driver. Set whenever the driver is interacted with (except on <tt>open()</tt> and <tt>close()</tt>) */
    u8 osf;  /**< Oscillator stop flag of the real-time-clock chip (updated on every read/write operation) */
    s8 temp;  /**< Temperature of the real-time-clock chip (updated on every read/write operation) */
} ds3231_status_t;

extern ds3231_status_t ds3231_status;

/**
 * Opens a connection to the I2C bus for communicating with the DS3231 RTC.
 * Connects to the RTC with address <tt>0x68</tt> on the I2C bus of the system
 * and creates a new I2C adapter and device. Also registers as an I2C driver.
 *
 * @brief Initializes the I2C driver.
 * @return <tt>0</tt> on success and either <tt>-ENODEV</tt> or the return
 * value of <tt>i2c_add_driver(i2c_driver*)</tt> on failure.
 *
 * @see i2c_add_driver(i2c_driver*)
 */
int ds3231_hw_init(void);

/**
 * Deletes the I2C driver from the system and unregisters the I2C device.
 */
void ds3231_hw_exit(void);

/**
 * Creates and registers the linux character driver for the ds3231 real-time-clock.
 * The driver will be created with the name <tt>ds3231_drv</tt> and will allocate
 * only <tt>1</tt> minor number for the character device.
 *
 * @return <tt>0</tt> on success and a kernel error code on failure. Should <tt>alloc_chrdev_region</tt> fail
 * it's return value will be returned. Otherwise <tt>-EIO</tt> will be returned.
 * @brief Creates the character device
 */
int ds3231_io_init(void);

/**
 * Destroys the character device and class and unregisters the character device
 * driver from the system. Also unregisters the character device region. Always
 * succeeds.
 *
 * @brief Destroys and frees all data associated with the character device driver
 */
void ds3231_io_exit(void);

/**
 * Configures the real-time-clock for driver usage by <ol><li>Disabling interrupts
 * and alarms</li><li>Re-enabling the oscillator and</li><li>setting the RTC to
 * 24hr mode</li></ol>
 * This method is called by the linux kernel.
 *
 * @brief Sets up the real-time-clock.
 * @param[in] client The I2C client for communicating with the RTC
 * @param[in] id The device id of the RTC (unused)
 * @return <tt>0</tt> on success and <tt>-ENODEV</tt> on failure.
 */
int ds3231_hw_probe(struct i2c_client *client, const struct i2c_device_id *id);

/**
 * Does nothing except write a debug message to the kernel log, just here
 * for completeness.
 * This method is called by the linux kernel.
 *
 * @param[in] client The I2C client being removed
 * @return <tt>0</tt>. This function never fails.
 */
int ds3231_hw_remove(struct i2c_client *client);

/**
 * Does nothing except write a debug message to the kernel log, just here
 * for completeness.
 * This method is called by the linux kernel.
 *
 * @param[in] inode The linux VFS inode for file-system access
 * @param[in] file The file handle to store specific data and provide information
 * on how that file was opened.
 * @return <tt>0</tt>. This function never fails
 */
int ds3231_io_open(struct inode *inode, struct file *file);

/**
 * Does nothing except write a debug message to the kernel log, just here
 * for completeness.
 * This method is called by the linux kernel.
 *
 * @param[in] inode The linux VFS inode for file-system access
 * @param[in] file The file handle to store specific data and provide information
 * on how that file was opened.
 * @return <tt>0</tt>. This function never fails
 */
int ds3231_io_close(struct inode *inode, struct file *file);

/**
 * Reads time and status from the RTC chip and writes it to a user-controlled character
 * device. It will be written to the character device in the format <tt>DD. M HH:MM:SS YYYY</tt>
 * where <tt>M</tt> refers to the full month name in German (see <tt>MONTH_NAMES</tt>).
 * Returns a kernel error code if either the Driver is already busy or the reading
 * from the chip fails.
 *
 * This method is called by the linux kernel.
 *
 * @brief Reads time and status from chip and writes  it to a user-controlled character device.
 * @param[in] file Struct that contains information about the caller and the type of call
 * @param[out] buffer Memory in userspace the data is to be written
 * @param[in] bytes number of bytes to be read
 * @param[in] offset offset inside the file or device
 * @return <ul><li><tt>-EBUSY</tt> if the driver is currently busy,</li><li><tt>-EAGAIN</tt> if the RTC's
 * oscillator was stopped (see <tt>ds3231_read_status(void)</tt>),</li><li>If there was
 * an error writing to the RTC <tt>-ENODEV</tt> is returned.</li></ul>otherwise the number of bytes left uncopied.
 *
 * @see MONTH_NAMES
 * @see ds3231_read_status(void)
 * @see ds3231_read_time(ds3231_time_t*)
 */
ssize_t ds3231_io_read(struct file *file, char __user *buffer, size_t bytes, loff_t *offset);

/**
 * Reads a time or temperature from userspace and writes it to the RTC-Chip.
 * Returns a kernel error code if either the driver is already busy, the
 * oscillator of the RTC stopped, the time provided is in a wrong format or
 * writing the time to the RTC failed.
 *
 * This method is called by the linux kernel.
 *
 * @brief Reads a time or temperature from userspace and writes it to the RTC-Chip.
 * @param[in] file Struct that contains information about the caller and the type of call
 * @param[in] buffer Space in userspace the data is read from
 * @param[in] bytes number of bytes to be written
 * @param[in] offset offset inside the file or device
 * @return <ul><li><tt>-EBUSY</tt> if the driver is currently busy,</li><li><tt>-ENOEXEC</tt> if
 * the format of the provided string is incorrect,</li><li><tt>-EAGAIN</tt> if the RTC's
 * oscillator was stopped (see <tt>ds3231_read_status(void)</tt>),
 * </li><li><tt>-EINVAL</tt> if the fields of the input are out of range (ie. 78 seconds),
 * </li><li><tt>-EOVERFLOW</tt> if the provided date is before the year 2000 or after the
 * year 2199.</li><li>If there was an error writing to the RTC <tt>-ENODEV</tt> is returned.</li></ul>
 * Otherwise the number of bytes processed is returned.
 *
 * @see ds3231_read_status(void)
 * @see ds3231_write_time(ds3231_time_t*)
 */
ssize_t ds3231_io_write(struct file *file, const char __user *buffer, size_t bytes, loff_t *offset);

/**
 * Writes the time stored in the parameter to the DS3231 RTC chip via the I2C bus.
 * It first converts the given decimal representation of the time into DS3231-readable
 * time according to the <tt>Timekeeping Registers</tt> on page 11 of the DS3231 manual.
 * It then writes these converted values to the DS3231 using the I2C bus. Years are written beginning
 * from <tt>0</tt> where <tt>0</tt> refers to the year <tt>2000</tt>.
 *
 * @param[in] time The time to write to the RTC
 * @return <tt>0</tt> on success and a kernel error code (returned by <tt>i2c_smbus_write_byte_data</tt>)
 * on failure.
 *
 * @see i2c_smbus_write_byte_data(i2c_client*, u8, u8)
 */
int ds3231_write_time(ds3231_time_t *time);

/**
 * Reads the time from the DS3231 RTC chip into the given <tt>ds3231_time_t</tt> object.
 * First this function reads all needed data registers from the RTC, then converts them
 * back to decimal values (according the <tt>Timekeeping Registers</tt> on page 11 of
 * the DS3231 manual) to return to the caller. Because years are stored as beginning
 * from <tt>0</tt>, <tt>2000</tt> is added to compensate for that.
 *
 * @param[out] time Where to write the time to.
 * @return <tt>0</tt> on success and a kernel error code (returned by <tt>i2c_smbus_read_byte_data</tt>)
 * on failure.
 *
 * @see i2c_smbus_read_byte_data(i2c_client*, u8)
 */
int ds3231_read_time(ds3231_time_t *time);

int ds3231_read_status(void);
