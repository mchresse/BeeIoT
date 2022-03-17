#ifndef MAIN_I2CDEV_H_
#define MAIN_I2CDEV_H_

#include "driver/i2c.h"

#define I2C_FREQ_HZ 	100000 	// Max 1MHz for esp32
#define I2CDEV_TIMEOUT	2000		// ms

// I2C Address Ports
#define ADS111X_ADDR_GND 0x48 //!< I2C device address with ADDR pin connected to ground
#define ADS111X_ADDR_VCC 0x49 //!< I2C device address with ADDR pin connected to VCC
#define ADS111X_ADDR_SDA 0x4a //!< I2C device address with ADDR pin connected to SDA
#define ADS111X_ADDR_SCL 0x4b //!< I2C device address with ADDR pin connected to SCL

// I2C Masterport for BIoT:
//     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
//00:          -- -- -- -- -- -- -- -- -- -- -- -- --
//10: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
//20: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
//30: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
//40: -- -- -- -- -- -- -- -- 48 -- -- -- -- -- -- --
//50: -- -- -- 53 -- -- -- -- -- -- -- -- -- -- -- --
//60: -- -- -- -- -- -- -- -- 68 -- -- -- -- -- -- --
//70: -- -- -- -- -- -- -- --

#define ADS_ADDR  	ADS111X_ADDR_GND	// ADC ADS1115
#define MAX_ADDR	0x34	// ADC MAX123x
#define RTC_ADDR 	0x68	// RTC DS3231
#define RTCF_ADDR	0x53	// RTC DS3231 FlashRam

// preselection of used ADC port
#define ADC_ADDR	ADS_ADDR		// Select Device MAX or ADS by I2C_ADDRESS

// driver/i2c lib definitions:
#define WRITE_BIT I2C_MASTER_WRITE // !< I2C master write
#define READ_BIT  I2C_MASTER_READ  // !< I2C master read
#define ACK_CHECK_EN	    0x1
#define ACK_CHECK_DIS       0x0
#define ACK_VAL             0x0    // !< I2C ack value
#define NACK_VAL			0x1


typedef struct {
    i2c_port_t	port;            // I2C port number
    uint8_t		addr;            // I2C address
    gpio_num_t	sda_io_num;      // GPIO number for I2C sda signal
    gpio_num_t	scl_io_num;      // GPIO number for I2C scl signal
    uint32_t	clk_speed;       // I2C clock frequency for master mode
} i2c_dev_t;

int 	  setup_i2c_master(int reentry);
esp_err_t i2c_master_init(i2c_port_t port, gpio_num_t sda, gpio_num_t scl);
esp_err_t i2c_dev_read	 (const i2c_dev_t *dev, const void *out_data, size_t out_size, void *in_data, size_t in_size);
esp_err_t i2c_dev_write	 (const i2c_dev_t *dev, const void *out_reg, size_t out_reg_size, const void *out_data, size_t out_size);
int 	  i2c_scan		 (void);

inline esp_err_t i2c_dev_read_reg(const i2c_dev_t *dev, uint8_t reg,
        void *in_data, size_t in_size)
{
    return i2c_dev_read(dev, &reg, 1, in_data, in_size);
}

inline esp_err_t i2c_dev_write_reg(const i2c_dev_t *dev, uint8_t reg,
        const void *out_data, size_t out_size)
{
    return i2c_dev_write(dev, &reg, 1, out_data, out_size);
}
#endif /* MAIN_I2CDEV_H_ */
