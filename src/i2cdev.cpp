/**
 * @file i2cdev.c
 *
 * ESP-IDF I2C master thread-safe functions for communication with I2C slave
 *
 * Copyright (C) 2018 Ruslan V. Uss <https://github.com/UncleRus>
 *
 * MIT Licensed as described in the file LICENSE
 */

//*******************************************************************
//
//   BeeIoT I2C-address mapping for ADS11x5 & RTC Module:
//      0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
// 00:          -- -- -- -- -- -- -- -- -- -- -- -- --
// 10: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
// 20: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
// 30: -- -- -- -- 34 -- -- -- -- -- -- -- -- -- -- --
// 40: -- -- -- -- -- -- -- -- 48 -- -- -- -- -- -- --
// 50: -- -- -- 53 -- -- -- -- -- -- -- -- -- -- -- --
// 60: -- -- -- -- -- -- -- -- 68 -- -- -- -- -- -- --
// 70: -- -- -- -- -- -- -- --
//*******************************************************************
// For ESP32-DevKitC PIN Configuration look at BeeIoT.h

#include <string.h>
#include <time.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/i2c.h"
#include "esp_log.h"

#include "i2cdev.h"
#include "max123x.h"
#include "i2cdev.h"

#include "beeiot.h"


extern uint16_t	lflags; // BeeIoT log flag field

// I2C Master Port: default = NUM0 using SDA/SCL = 21/22
// to be referenced by all I2C driver routines
i2c_port_t 	 i2c_master_port = I2C_PORT ;
i2c_config_t i2ccfg;		// Configset of I2C MasterPort

int isi2c = 0;					// =0 no I2C master port claimed yet
RTC_DATA_ATTR int adcaddr=0;	// I2C Dev.address of detected ADC
RTC_DATA_ATTR int isadc=0;		// =0 no I2C dev found at i2c_master_port
extern int isrtc;				// in rtc.cpp

esp_err_t i2c_master_init(i2c_port_t port, gpio_num_t sda, gpio_num_t scl){
	esp_err_t	esprc;
	i2c_config_t cfg;

	// ESP IDF I2C port configuration object
	i2ccfg.mode 		 = I2C_MODE_MASTER;
	i2ccfg.sda_io_num 	 = sda;
	i2ccfg.scl_io_num 	 = scl;
	i2ccfg.sda_pullup_en = GPIO_PULLUP_ENABLE;
	i2ccfg.scl_pullup_en = GPIO_PULLUP_ENABLE;
	i2ccfg.master.clk_speed = I2C_FREQ_HZ;

	i2c_param_config(port, &i2ccfg);
	esprc = i2c_driver_install(port, I2C_MODE_MASTER, 0, 0, 0);
	return(esprc);
}

int setup_i2c_master(int reentry) {
	esp_err_t	esprc;

	BHLOG(LOGADS) Serial.printf("  I2C: Init I2C-Master port %d\n", I2C_PORT);
	pinMode(I2C_SCL, OUTPUT);		// prepare Alert input line of connected ADS1511S at I2C Bus
    digitalWrite(I2C_SCL, HIGH);	// define default level

	gpio_hold_dis((gpio_num_t) I2C_SCL);  	// enable ADS_SCL for Dig.-IO I2C Master Mode
	gpio_hold_dis((gpio_num_t) I2C_SDA);  	// enable ADS_SDA for Dig.-IO I2C Master Mode

	isi2c = 0;
	esprc=i2c_master_init(I2C_PORT, I2C_SDA, I2C_SCL);
	if(esprc == ESP_OK){
		i2c_master_port = I2C_PORT;	// I2C master Port driver initiated
		isi2c = 1;	// we have a working I2C master port
		if(!reentry){
			isi2c = i2c_scan();		// Discover I2C dev. Addresses once
		}
	}
	return(isi2c);
} // end of setup_i2c_master()

esp_err_t i2c_dev_read(const i2c_dev_t *dev, const void *out_data, size_t out_size, void *in_data, size_t in_size)
{
    if (!dev || !in_data || !in_size) return ESP_ERR_INVALID_ARG;

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    if (out_data && out_size)
    {
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (dev->addr << 1) | I2C_MASTER_WRITE, true);
        i2c_master_write(cmd, (uint8_t *)out_data, out_size, true);
    }
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (dev->addr << 1) | I2C_MASTER_READ, true);
    i2c_master_read(cmd, (uint8_t*)in_data, in_size, I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);

    esp_err_t res = i2c_master_cmd_begin(dev->port, cmd, I2CDEV_TIMEOUT / portTICK_RATE_MS);
    if (res != ESP_OK)
        BHLOG(LOGADS) Serial.printf("  I2C: Could not read from device [0x%02x at %d]: %d", dev->addr, dev->port, res);
    i2c_cmd_link_delete(cmd);

    return res;
}


esp_err_t i2c_dev_write(const i2c_dev_t *dev, const void *out_reg, size_t out_reg_size, const void *out_data, size_t out_size)
{
    if (!dev || !out_data || !out_size) return ESP_ERR_INVALID_ARG;

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (dev->addr << 1) | I2C_MASTER_WRITE, true);
    if (out_reg && out_reg_size)
        i2c_master_write(cmd, (uint8_t*)out_reg, out_reg_size, true);
    i2c_master_write(cmd, (uint8_t*)out_data, out_size, true);
    i2c_master_stop(cmd);
    esp_err_t res = i2c_master_cmd_begin(dev->port, cmd, I2CDEV_TIMEOUT / portTICK_RATE_MS);
    if (res != ESP_OK)
        BHLOG(LOGADS) Serial.printf("  ADS: Could not write to device [0x%02x at %d]: %d", dev->addr, dev->port, res);
    i2c_cmd_link_delete(cmd);

    return res;
}

//***************************************************************
//* I2C_Test()
//* Test of I2C device Scanner
//  Return: =0	No I2C dev detected
//			=1  I2C Device(s) detected -> isadc or isrtc
//***************************************************************
int i2c_scan(void) {
  	isadc = 0;
	isrtc = 0;

	if(!isi2c)		// no I2C Master Port Init -> no Scan
		return(0);

	int i;
	esp_err_t esprc;
	i2c_cmd_handle_t cmd;
	Serial.printf("     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f\n");
	Serial.printf("00:         ");

	for (i=3; i< 0x78; i++) {
		cmd = i2c_cmd_link_create();
		i2c_master_start(cmd);
		i2c_master_write_byte(cmd, (i << 1) | I2C_MASTER_WRITE, ACK_CHECK_EN /* expect ack */);
		i2c_master_stop(cmd);

		esprc = i2c_master_cmd_begin(i2c_master_port, cmd, 10/portTICK_PERIOD_MS);
		if (i%16 == 0) {
			Serial.printf("\n%.2x:", i);
		}
		if (esprc == 0) {
			Serial.printf(" %.2x", i);
			// if ADC: ADS111x overrules MAX123x by intention asking in that order
			if(i==MAX_ADDR) {    
				isadc=1; adcaddr = i;	// remember ADS I2C dev.addr
			} 
			if (i == ADS111X_ADDR_GND || i == ADS111X_ADDR_VCC ||
    			i == ADS111X_ADDR_SDA || i == ADS111X_ADDR_SCL){
				isadc=1; adcaddr = i;	// remember MAX I2C dev.addr
			}  
			if(i==RTC_ADDR) {
				isrtc=i;				// remember RTC Presence -> use RTC_ADDR
			}    
		} else {
			Serial.printf(" --");
		}

		//ESP_LOGD(tag, "i=%d, rc=%d (0x%x)", i, espRc, espRc);
		i2c_cmd_link_delete(cmd);
	}

	Serial.printf("\n");

	return(isadc);
}
