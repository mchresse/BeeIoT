//*******************************************************************
// i2c_ads.cpp
// from Project https://github.com/mchresse/BeeIoT
//
// Description:
// Contains main setup() and loop() routines for I2C sensosrs.
//
//-------------------------------------------------------------------
// Copyright (c) 10/2019-present, Randolph Esser
// All rights reserved.
// This file is distributed under the BSD-3-Clause License
// The complete license agreement can be obtained at:
//     https://github.com/mchresse/BeeIoT/license
// For used 3rd party open source see also Readme_OpenSource.txt
//*******************************************************************
//

#include <Arduino.h>
#include <stdio.h>
// #include <esp_log.h>
// #include "FS.h"
// #include "SD.h"

#include "beeiot.h"     	// provides all GPIO PIN configurations of all sensor Ports !

//*******************************************************************
// ADS1115 I2C Library
//*******************************************************************
#include <driver/i2c.h>
#include "i2cdev.h"
#include "i2c_ads.h"

// #include <Wire.h>
#include <Adafruit_ADS1x15.h>
//Adafruit_ADS1115 ads(ADS_ADDR); 		// 16-bit ADC version, ADDR pin = Gnd

extern uint16_t	lflags;      			// BeeIoT log flag field
extern int isadc;						// in max123x.cpp
extern int isi2c;						// in i2cdev.cpp
extern int adcaddr;						// I2C Dev.address of detected ADC
extern i2c_port_t i2c_master_port;		// I2C master Port ID, defined in i2cdev.cpp
extern i2c_config_t i2ccfg;				// Configset of I2C MasterPort

static i2c_dev_t i2cads1115;			// Config settings of detected I2C device
const float 	 adcfactor = 0.1875F;	// factor for 6.144V range

const float ads111x_gain_values[] = {
    [ADS111X_GAIN_6V144]   = 6.144,
    [ADS111X_GAIN_4V096]   = 4.096,
    [ADS111X_GAIN_2V048]   = 2.048,
    [ADS111X_GAIN_1V024]   = 1.024,
    [ADS111X_GAIN_0V512]   = 0.512,
    [ADS111X_GAIN_0V256]   = 0.256,
    [ADS111X_GAIN_0V256_2] = 0.256,
    [ADS111X_GAIN_0V256_3] = 0.256
};


static esp_err_t read_reg(i2c_dev_t *dev, uint8_t reg, uint16_t *val);
static esp_err_t write_reg(i2c_dev_t *dev, uint8_t reg, uint16_t val);
static esp_err_t write_conf_bits(i2c_dev_t *dev, uint16_t val, uint8_t offs, uint16_t mask);
static esp_err_t read_conf_bits(i2c_dev_t *dev, uint8_t offs, uint16_t mask, uint16_t *bits);
static esp_err_t READ_CONFIG(i2c_dev_t *dev, uint8_t OFFS, uint16_t MASK, uint16_t * VAR);
uint16_t ads_read(int channel);

//*******************************************************************
// I2c-ADS Setup Routine
// The ADC input range (or gain) can be changed via the following
// functions, but be careful never to exceed VDD +0.3V max, or to
// exceed the upper and lower limits if you adjust the input range!
// Setting these values incorrectly may destroy your ADC!
//                                                                ADS1015  ADS1115
//                                                                -------  -------
// ads.setGain(GAIN_TWOTHIRDS);  // 2/3x gain +/- 6.144V  1 bit = 3mV      0.1875mV (default)
// ads.setGain(GAIN_ONE);        // 1x gain   +/- 4.096V  1 bit = 2mV      0.125mV
// ads.setGain(GAIN_TWO);        // 2x gain   +/- 2.048V  1 bit = 1mV      0.0625mV
// ads.setGain(GAIN_FOUR);       // 4x gain   +/- 1.024V  1 bit = 0.5mV    0.03125mV
// ads.setGain(GAIN_EIGHT);      // 8x gain   +/- 0.512V  1 bit = 0.25mV   0.015625mV
// ads.setGain(GAIN_SIXTEEN);    // 16x gain  +/- 0.256V  1 bit = 0.125mV  0.0078125mV
//*******************************************************************


int setup_i2c_ADS(int reentry) {  // ADS1115S constructor
#ifdef  ADS_CONFIG
	uint16_t val;
	BHLOG(LOGADS) Serial.printf("  ADS: Init I2C ADS device & Alert line\n");
	pinMode(ADS_ALERT, INPUT);		// prepare Alert input line of connected ADS1511S at I2C Bus
	gpio_hold_dis((gpio_num_t) ADS_ALERT);  // enable ADS_ALERT for Dig.-IO I2C Master Mode

	//	I2C should have been scanned by I2c_master_init() already
	if (adcaddr == ADS111X_ADDR_GND || adcaddr == ADS111X_ADDR_VCC ||
    	adcaddr == ADS111X_ADDR_SDA || adcaddr == ADS111X_ADDR_SCL){
		BHLOG(LOGADS) Serial.printf("  ADS: ADC ADS1115 detected at port: 0x%02X\n", adcaddr);
	}else{
		// no ADS111x found
		return(0);
	}

	// Setup MAX I2C device statically
	// Should not be scanned by i2c_scan() all the time after each sleep-loop
	i2cads1115.addr 	 = adcaddr;
	i2cads1115.clk_speed = i2ccfg.master.clk_speed;
	i2cads1115.port 	 = i2c_master_port;	// as defined in i2c_master_init()
	i2cads1115.scl_io_num= i2ccfg.scl_io_num;
	i2cads1115.sda_io_num= i2ccfg.sda_io_num;

    // change GAIN level if needed
	if( ads111x_set_gain(&i2cads1115, ADS111X_GAIN_6V144) != ESP_OK){	// set gain to minimum
        BHLOG(LOGADS) Serial.printf("  ADS: No reaction from ADS dev(%X)\n", adcaddr);
		isadc = 0;		// doesn't work
		return(isadc);	// no I2C master port -> no ADS1115 Device
	}

	BHLOG(LOGADS) read_reg(&i2cads1115, REG_CONFIG, &val);
	BHLOG(LOGADS) Serial.printf("  ADS: Got Default config value: 0x%04x\n", val);

#endif //ADS_CONFIG

  return isadc;   // I2C-ADS device port is initialized
} // end of setup_i2c_ADS()


//***************************************************************
// ADS_read()
//  Read ADS1115 I2C analog port
// based on espressif i2c driver lib
//
// 	IN:   channel = 0..3
//  OUT:  data  = voltage of adc line [mV]
//
//***************************************************************
uint16_t ads_read(int channel) {
	esp_err_t esprc=ESP_ERR_NOT_SUPPORTED;
	float voltage = 0;

#ifdef ADS_CONFIG
	int16_t adcdata;
	ads111x_mux_t chn;
    bool busy;
	static float gain_val;		// Gain value

    BHLOG(LOGADS) Serial.printf("\n  ADS: Read from ADS-AIN%d: ", channel);
	switch (channel){
		case 0:	chn = ADS111X_MUX_0_GND; break;
		case 1:	chn = ADS111X_MUX_1_GND; break;
		case 2:	chn = ADS111X_MUX_2_GND; break;
		case 3:	chn = ADS111X_MUX_3_GND; break;
		default: return(ESP_ERR_NOT_SUPPORTED);
	} // get channel value for Cfg register

	esprc = ads111x_set_mode(&i2cads1115, ADS111X_MODE_SINGLE_SHOT);
//	BHLOG(LOGADS) Serial.printf(" Mode=%02x  rc1=%i ", ADS111X_MODE_SINGLE_SHOT, esprc);
	esprc = ads111x_set_data_rate(&i2cads1115, ADS111X_DATA_RATE_32); // 32 samples per second
//	BHLOG(LOGADS) Serial.printf(" DRate=%02x rc2=%i ", ADS111X_DATA_RATE_32, esprc);
	esprc = ads111x_set_input_mux(&i2cads1115, chn);
//	BHLOG(LOGADS) Serial.printf(" Mux=%02x   rc3=%i ", chn, esprc);
	esprc = ads111x_set_gain(&i2cads1115, ADS111X_GAIN_6V144);
//	BHLOG(LOGADS) Serial.printf(" Gain=%02x  rc4=%i ", ADS111X_GAIN_6V144, esprc);
	esprc = ads111x_start_conversion(&i2cads1115);
//	BHLOG(LOGADS) Serial.printf(" Start Conversion (rc5=%i)\n", esprc);
    do{	// wait for conversion end
		BHLOG(LOGADS) Serial.print(".");
        ads111x_is_busy(&i2cads1115, &busy);
    } while (busy==true);

	esprc = ads111x_get_value(&i2cads1115, &adcdata);
    if (esprc != ESP_OK) {
		BHLOG(LOGADS) Serial.printf(" failed rc=%i\n", esprc);
        return esprc;
    }

	gain_val = ads111x_gain_values[ADS111X_GAIN_6V144];
 	voltage = (gain_val / ADS111X_MAX_VALUE * adcdata) * 1000;	// get value in mV
//	BHLOG(LOGADS) Serial.printf("     Raw ADC value: %d, voltage: %.04f volts   ", adcdata, voltage);

	// Result with ADC int. Reference: +/- 6.144V
	// ADS1015: 11bit ADC -> 6144/2048  = 3 mV / bit
	// ADS1115: 15bit ADC -> 6144/32768 = 0.1875 mV / bit
//	data = (float)adcdata * adcfactor;	// multiply by 1bit-sample in mV
//	BHLOG(LOGADS) Serial.printf("  data:%i -> %.3fmV\n", adcdata, data);

#endif // ADS_CONFIG
    return ((uint16_t) voltage);	// Value in mV
}




//***************************************************************
// ads Helper functions
// 	based on espressif i2cdev driver lib
//***************************************************************
static esp_err_t read_reg(i2c_dev_t *dev, uint8_t reg, uint16_t *val)
{
    uint8_t buf[2];
    esp_err_t res;
    if ((res = i2c_dev_read_reg(dev, reg, buf, 2)) != ESP_OK)
    {
        BHLOG(LOGADS) Serial.printf("  ADS: Could not read from register 0x%02x\n", reg);
        return res;
    }
    *val = (buf[0] << 8) | buf[1];
//        BHLOG(LOGADS) Serial.printf("  ADS: RD-Reg%d 0x%04X\n", reg, *val);

    return ESP_OK;
}

static esp_err_t write_reg(i2c_dev_t *dev, uint8_t reg, uint16_t val){
    esp_err_t res;
	uint8_t buf[2];

	buf[1] =  (uint8_t)(val & 0xFF);
	buf[0] =  (uint8_t)(val >> 8);

    if ((res = i2c_dev_write_reg(dev, reg, buf, 2)) != ESP_OK)
    {
        BHLOG(LOGADS) Serial.printf("  ADS: Could not write 0x%02x%02x to register 0x%02x\n", buf[0], buf[1], reg);
        return res;
    }
//    BHLOG(LOGADS) Serial.printf("  ADS: WR-Reg%d < 0x%02x%02x\n", reg, buf[0], buf[1]);

    return ESP_OK;
}

static esp_err_t read_conf_bits(i2c_dev_t *dev, uint8_t offs, uint16_t mask, uint16_t *bits){
	esp_err_t esprc;
    uint16_t val;

 	esprc = read_reg(dev, REG_CONFIG, (uint16_t*) &val);

    *bits = (val >> offs) & mask;
	//	BHLOG(LOGADS) Serial.printf("  ADS: RD config: 0x%04x (0x%04X) (o:%i, m:0x%04x)\n", val, *bits, offs, mask);
    return(esprc);
}

static esp_err_t write_conf_bits(i2c_dev_t *dev, uint16_t val, uint8_t offs,
uint16_t mask){
	esp_err_t 	esprc;
    uint16_t 	old;

	read_reg(dev, REG_CONFIG, &old);
	old = old & 0x7FFF;		// mask OS Bit -> set at start_conversion() only
	esprc = write_reg(dev, REG_CONFIG, (old & ~(mask << offs)) | (val << offs));

    return(esprc);
}

static esp_err_t READ_CONFIG(i2c_dev_t *dev, uint8_t OFFS, uint16_t MASK, uint16_t * bits){
	return( read_conf_bits(dev, OFFS, MASK, bits));
}

esp_err_t ads111x_is_busy(i2c_dev_t *dev, bool *busy){
	esp_err_t 	esprc;
	uint16_t 	bits;

    esprc = read_conf_bits(dev, OS_OFFSET, OS_MASK, &bits);
	if(bits == 0){
		*busy = true;	// conversion still running
	}else{
		*busy = false;	// done -> idle
	}
	return(esprc);
}

esp_err_t ads111x_start_conversion(i2c_dev_t *dev){
	esp_err_t 	esprc;
    uint16_t 	old;

	read_reg(dev, REG_CONFIG, &old);
	esprc = write_reg(dev, REG_CONFIG, (old | ((uint16_t) 1 << OS_OFFSET)));
    return esprc;
}

esp_err_t ads111x_get_value(i2c_dev_t *dev, int16_t *value){
	return( read_reg(dev, REG_CONVERSION, (uint16_t *)value) );
}

esp_err_t ads111x_get_gain(i2c_dev_t *dev, ads111x_gain_t *gain){
    return(READ_CONFIG(dev, PGA_OFFSET, PGA_MASK, (uint16_t*) gain));
}

esp_err_t ads111x_set_gain(i2c_dev_t *dev, ads111x_gain_t gain){
    return write_conf_bits(dev, gain, PGA_OFFSET, PGA_MASK);
}

esp_err_t ads111x_get_input_mux(i2c_dev_t *dev, ads111x_mux_t *mux){
    return(READ_CONFIG(dev, MUX_OFFSET, MUX_MASK, (uint16_t*)  mux));
}

esp_err_t ads111x_set_input_mux(i2c_dev_t *dev, ads111x_mux_t mux)
{
    return write_conf_bits(dev, mux, MUX_OFFSET, MUX_MASK);
}

esp_err_t ads111x_get_mode(i2c_dev_t *dev, ads111x_mode_t *mode)
{
    return(READ_CONFIG(dev, MODE_OFFSET, MODE_MASK, (uint16_t*) mode));
}

esp_err_t ads111x_set_mode(i2c_dev_t *dev, ads111x_mode_t mode)
{
    return write_conf_bits(dev, mode, MODE_OFFSET, MODE_MASK);
}

esp_err_t ads111x_get_data_rate(i2c_dev_t *dev, ads111x_data_rate_t *rate)
{
    return(READ_CONFIG(dev, DR_OFFSET, DR_MASK, (uint16_t*) rate));
}

esp_err_t ads111x_set_data_rate(i2c_dev_t *dev, ads111x_data_rate_t rate)
{
    return write_conf_bits(dev, rate, DR_OFFSET, DR_MASK);
}

esp_err_t ads111x_get_comp_mode(i2c_dev_t *dev, ads111x_comp_mode_t *mode)
{
    return(READ_CONFIG(dev, COMP_MODE_OFFSET, COMP_MODE_MASK, (uint16_t*) mode));
}

esp_err_t ads111x_set_comp_mode(i2c_dev_t *dev, ads111x_comp_mode_t mode)
{
    return write_conf_bits(dev, mode, COMP_MODE_OFFSET, COMP_MODE_MASK);
}

esp_err_t ads111x_get_comp_polarity(i2c_dev_t *dev, ads111x_comp_polarity_t *polarity)
{
    return(READ_CONFIG(dev, COMP_POL_OFFSET, COMP_POL_MASK, (uint16_t*) polarity));
}

esp_err_t ads111x_set_comp_polarity(i2c_dev_t *dev, ads111x_comp_polarity_t polarity)
{
    return write_conf_bits(dev, polarity, COMP_POL_OFFSET, COMP_POL_MASK);
}

esp_err_t ads111x_get_comp_latch(i2c_dev_t *dev, ads111x_comp_latch_t *latch)
{
    return(READ_CONFIG(dev, COMP_LAT_OFFSET, COMP_LAT_MASK, (uint16_t*) latch));
}

esp_err_t ads111x_set_comp_latch(i2c_dev_t *dev, ads111x_comp_latch_t latch)
{
    return write_conf_bits(dev, latch, COMP_LAT_OFFSET, COMP_LAT_MASK);
}

esp_err_t ads111x_get_comp_queue(i2c_dev_t *dev, ads111x_comp_queue_t *queue)
{
    return(READ_CONFIG(dev, COMP_QUE_OFFSET, COMP_QUE_MASK, (uint16_t*) queue));
}

esp_err_t ads111x_set_comp_queue(i2c_dev_t *dev, ads111x_comp_queue_t queue){
    return write_conf_bits(dev, queue, COMP_QUE_OFFSET, COMP_QUE_MASK);
}

esp_err_t ads111x_get_comp_low_thresh(i2c_dev_t *dev, int16_t *th){
	return( read_reg(dev, REG_THRESH_L, (uint16_t *)th));
}

esp_err_t ads111x_set_comp_low_thresh(i2c_dev_t *dev, int16_t th){
	return( write_reg(dev, REG_THRESH_L, th));
}

esp_err_t ads111x_get_comp_high_thresh(i2c_dev_t *dev, int16_t *th){
	return( read_reg(dev, REG_THRESH_H, (uint16_t *)th));
}

esp_err_t ads111x_set_comp_high_thresh(i2c_dev_t *dev, int16_t th){
	return( write_reg(dev, REG_THRESH_H, th));
}


//***************************************************************
// ADS_read()
//  Read ADS1115 I2C analog port
//  Based on Wire-lib implementation
//
// 	IN:   channel = 0..3
//  OUT:  data  = voltage of adc line [mV]
// see also
//  https://esp32developer.com/programming-in-c-c/i2c-programming-in-c-c/using-the-i2c-interface
// Setup:
// ads.setGain(GAIN_TWOTHIRDS);  +/- 6.144V  1 bit = 0.1875mV (default)
// ads.setGain(GAIN_ONE);        +/- 4.096V  1 bit = 0.125mV
// ads.setGain(GAIN_TWO);        +/- 2.048V  1 bit = 0.0625mV
// ads.setGain(GAIN_FOUR);       +/- 1.024V  1 bit = 0.03125mV
// ads.setGain(GAIN_EIGHT);      +/- 0.512V  1 bit = 0.015625mV
// ads.setGain(GAIN_SIXTEEN);    +/- 0.256V  1 bit = 0.0078125mV
//***************************************************************
uint16_t ads_read_wire(int channel) {
	int16_t data=0;
#ifdef ADS_CONFIG
// 	int16_t adcdata;

	BHLOG(LOGADS) Serial.printf("\n  ADS: Single-ended read from AIN%i: ", channel);
	channel &= 0x03;	// ADS1115 has only 4 channel -> 0..3
//	adcdata = ads.readADC_SingleEnded(channel); // read once in Single-shot mode (default)
	// Result with ADC int. Reference: +/- 6.144V
	// ADS1015: 11bit ADC -> 6144/2048  = 3 mV / bit
	// ADS1115: 15bit ADC -> 6144/32768 = 0.1875 mV / bit
// 	data = adcdata * adcfactor;	// multiply by 1bit-sample in mV
//	delay(100);

#endif // ADS_CONFIG
	return data;
}