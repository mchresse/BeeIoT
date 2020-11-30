

//*******************************************************************
// max123x.cpp
// from Project https://github.com/mchresse/BeeIoT
//
// Description:
// Contains main setup() and read() routines for MAX123x I2C sensors.
// supporting max1236/max1237 using I2C addr: 0x34
//-------------------------------------------------------------------
// Copyright (c) 11/2020-present, Randolph Esser
// All rights reserved.
// This file is distributed under the BSD-3-Clause License
// The complete license agreement can be obtained at:
//     https://github.com/mchresse/BeeIoT/license
// For used 3rd party open source see also Readme_OpenSource.txt
//*******************************************************************
//
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

#include <Arduino.h>
#include <stdio.h>
// #include <esp_log.h>
// #include "FS.h"
// #include "SD.h"

//*******************************************************************
// I2C Library
//*******************************************************************
//#include <Wire.h>
#include <driver/i2c.h>
#include "i2cdev.h"

#include "max123x.h"
#include "beeiot.h"     	// provides all GPIO PIN configurations of all sensor Ports !


extern uint16_t	lflags; 	// BeeIoT log flag field

extern int		isadc;    	// =0 no ADC found; =1 else MAX or ADS
extern int 		isi2c;		// in i2cdev.cpp)
extern i2c_port_t i2c_master_port;	// defined in i2cdev.cpp
extern i2c_config_t i2ccfg;		// Configset of I2C MasterPort
extern int 		adcaddr;	// I2C Dev.address of detected ADC

i2c_dev_t i2cmax123x;		// Config settings of detected MAX device

extern uint16_t ads_read(int channel);

//*******************************************************************
// MAX123x Setup Routine
// The standard Espressif I2C dreiver implementation is used!
// Hope it coexists with the Wire.h lib as used by the RTC DS3231
//*******************************************************************
int setup_i2c_MAX(int reentry) {  // MAX123x constructor

#ifdef  ADS_CONFIG
	BHLOG(LOGADS) Serial.printf("  MAX: Init I2C-port (addr: 0x%02X)\n", adcaddr);

	//	I2C should have been scanned by I2c_master_init() already
	if(adcaddr != MAX_ADDR){
       	BHLOG(LOGADS) Serial.println("  MAX: No MAX device detected");
		return(0);
    }

	// Setup MAX I2C device statically
	// Should not be scanned by i2c_scan() all the time after each sleep-loop
	i2cmax123x.addr 	 = adcaddr;
	i2cmax123x.clk_speed = i2ccfg.master.clk_speed;
	i2cmax123x.port 	 = i2c_master_port;	// as defined in i2c_master_init()
	i2cmax123x.scl_io_num= i2ccfg.scl_io_num;
	i2cmax123x.sda_io_num= i2ccfg.sda_io_num;
  #endif //ADS_CONFIG

  return(1);   // MAX123x device port is initialized
} // end of setup_i2c_MAX()



//***************************************************************
// ADC_read()
// 	Convert and read analog port from AD1115 or MAX123x I2C device
//  using Espressif IDF: i2cdev functionality from driver/i2c.h
//  https://esp32developer.com/programming-in-c-c/i2c-programming-in-c-c/using-the-i2c-interface
//	Device is selected by discovered I2C Port address: ADC_ADDR
//
// INPUT:
// 	channel	0..3	Index of AINx
// 	global adcaddr	I2C Addr of detected ADC device
//
// OUTPUT
//	data		ADC value in mV
// 				-> Vref=2096V data = n x 1/Vref
//***************************************************************
#define MAXCAL	7		// MAX Calibrationfaktor +7% 
#define ADSCAL	0		// ADS Calibrationfaktor +0% 
int16_t ADCCal;			// ADC Calibrationfaktor +7% 


uint16_t adc_read(int channel) {

    if(adcaddr == MAX_ADDR){
		ADCCal = MAXCAL;
      return(max_read(channel) * (100+ADCCal) / 100);
    }
    if (adcaddr == ADS111X_ADDR_GND || adcaddr == ADS111X_ADDR_VCC ||
    	adcaddr == ADS111X_ADDR_SDA || adcaddr == ADS111X_ADDR_SCL){
		ADCCal = ADSCAL;
      return(ads_read(channel) * (100+ADCCal) / 100);
    }
  return(-1);
}

//***************************************************************
// max_read()
// Convert and read selected Analog channel
// from MAX1236 4-port 12bit ADC
// INPUT:
// 	channel	0..3	Index of AINx
// OUTPUT
//	data	digital value -> Vref=2096V data = n x 1/2096
//************************************************************

uint16_t max_read(uint8_t channel) {
    int ret=0;
#ifdef ADS_CONFIG

    // setup byte:   Ref.int., Vref=InternalOn, int.clock, unipolar, no reset cfg.register
	uint8_t setupbyte 	= MAX1363_SETUPREG | MAX1363_SETUP_AIN3_IS_AIN3_REF_IS_INT		// Default: 0xD2
				| MAX1363_SETUP_POWER_UP_INT_REF
				| MAX1363_SETUP_INT_CLOCK | MAX1363_SETUP_UNIPOLAR | MAX1363_SETUP_NORESET ;
    // Send config byte: Mode=single ended, one sel. channel,
	uint8_t configbyte 	= MAX1363_CONFIGREG | MAX1363_CONFIG_SCAN_SINGLE_1 				// default 0: 0x61 (1: 0x63, 2: 0x65, 3: 0x67)
				| ((channel << 1) & MAX123x_CHANNEL_SEL_MASK)| MAX1363_CONFIG_SE;

    BHLOG(LOGADS) Serial.printf("  MAX-Adr(0x%02X): Write Setup 0x%02X", isadc, setupbyte);
    BHLOG(LOGADS) Serial.printf("-  Config: 0x%02X",configbyte);

// Start MAX1236 in F/S Mode
	ret = i2c_dev_write( &i2cmax123x, &setupbyte, 1, &configbyte, 1);
    if (ret != ESP_OK) {
    	BHLOG(LOGADS) Serial.printf(" -> RC(%i)\n",ret);
        return(-1);   // Initialization was not successfull -> skip
    }

// Start Read Conversion data
	int8_t datax[2] ={0,0};
	int data=0;
	ret = i2c_dev_read( &i2cmax123x, 0,0, &datax[0], 2 );	// no out data
    if (ret != ESP_OK) {
		BHLOG(LOGADS) Serial.printf(" => Read data failed ->  RC(%i)\n",ret);
	}else{
		data = ((datax[0] & 0x000F)*256 + (datax[1] & 0x00FF));
		BHLOG(LOGADS) Serial.printf("-> done: 0x%02x-%02x (%i)\n",  datax[0] &0x000F, datax[1] & 0x00FF, data);
	}
	data = data * 1000 / 2096;	// get value in mV
    return((uint16_t) data);	// get 12 bit conversion word

#endif // ADS_CONFIG
}


//************************************************************************
// Not used by now !!!

uint16_t max_read_core(uint8_t channel) {
    int ret=0;
#ifdef ADS_CONFIG

	uint8_t		data_h;		// result high Byte
	uint8_t		data_l;		// result low Byte
	uint8_t		txbuf[4];	// I2C transfer buffer

	uint8_t		setupbyte;	// setup register value
	uint8_t		configbyte;	// config register value

    // setup byte:   Ref.int., Vref=InternalOn, int.clock, unipolar, no reset cfg.register
	setupbyte 	= MAX1363_SETUPREG | MAX1363_SETUP_AIN3_IS_AIN3_REF_IS_INT		// Default: 0xD2
				| MAX1363_SETUP_POWER_UP_INT_REF
				| MAX1363_SETUP_INT_CLOCK | MAX1363_SETUP_UNIPOLAR | MAX1363_SETUP_NORESET ;
    // Send config byte: Mode=single ended, one sel. channel,
	configbyte 	= MAX1363_CONFIGREG | MAX1363_CONFIG_SCAN_SINGLE_1 				// default 0: 0x61 (1: 0x63, 2: 0x65, 3: 0x67)
				| ((channel << 1) & MAX123x_CHANNEL_SEL_MASK)| MAX1363_CONFIG_SE;

	txbuf[0]	= (adcaddr << 1) | WRITE_BIT;	// device select by I2C address + Write Flag
	txbuf[1]	= setupbyte;
	txbuf[2]	= configbyte;
    Serial.printf("  MAX-Adr(0x%02X): Write Setup 0x%02X", isadc, setupbyte);
    Serial.printf("-  Config: 0x%02X",configbyte);

// Start MAX1236 in F/S Mode
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();	// get I2C Dev. Handle

	// define new I2C Message: Init MAX device
    i2c_master_start(cmd);		// Create new task QUEUE
    i2c_master_write_byte(cmd, txbuf[0],   (i2c_ack_type_t) ACK_CHECK_EN);		// Write /w ACK
    i2c_master_write_byte(cmd, setupbyte,  (i2c_ack_type_t) ACK_CHECK_EN);		// Write /w ACK
    i2c_master_write_byte(cmd, configbyte, (i2c_ack_type_t) ACK_CHECK_EN);		// Write /w ACK
    i2c_master_stop(cmd);   	// end task creation

    // start conversion by processing cmd queue above
    ret = i2c_master_cmd_begin(i2cmax123x.port, cmd, 1000 / portTICK_RATE_MS);
    i2c_cmd_link_delete(cmd);

    if (ret != ESP_OK) {
    	Serial.printf(" -> RC(%i)\n",ret);
        return(-1);   // Initialization was not successfull -> skip
    }
	// Slave side clock stretching let I2C comm. master wait for conversion
//    delay(100);  // wait 30ms:  vTaskDelay(30 / portTICK_RATE_MS);

// Start Read Conversion data
	txbuf[0]	= (adcaddr << 1) | READ_BIT;	// device select by I2C address + Write Flag
    cmd = i2c_cmd_link_create();				// get I2C Dev. Handle
    i2c_master_start(cmd);						// start filling task QUEUE

    i2c_master_write_byte(cmd, txbuf[0],(i2c_ack_type_t) ACK_CHECK_EN);		// /w ACK
    i2c_master_read_byte (cmd, &data_h, (i2c_ack_type_t) ACK_VAL);
	i2c_master_read_byte (cmd, &data_l, (i2c_ack_type_t) NACK_VAL);  // last read /wo Ack

	i2c_master_stop(cmd);

	ret = i2c_master_cmd_begin(i2cmax123x.port , cmd, 1000 / portTICK_RATE_MS);
	i2c_cmd_link_delete(cmd);

  	Serial.printf(" => Read data ->  RC(%i)\n",ret);
//  Serial.printf("  MAX-Port(%d): %i-%i", channel, data_h, data_l);

    return((uint16_t) (data_l + data_h * 256) & 0x0FFF);	// get 12 bit conversion word

#endif // ADS_CONFIG
}


