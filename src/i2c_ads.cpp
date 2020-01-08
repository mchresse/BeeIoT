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
// This Module contains code derived from
// - HX711 library example code
//*******************************************************************
//
//   BeeIoT I2C-address mapping for ADS11x5 & RTC Module:
//      0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
// 00:          -- -- -- -- -- -- -- -- -- -- -- -- --
// 10: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
// 20: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
// 30: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
// 40: -- -- -- -- -- -- -- -- 48 -- -- -- -- -- -- --
// 50: -- -- -- 53 -- -- -- -- -- -- -- -- -- -- -- --
// 60: -- -- -- -- -- -- -- -- 68 -- -- -- -- -- -- --
// 70: -- -- -- -- -- -- -- --
//*******************************************************************
// For ESP32-DevKitC PIN Configuration look at BeeIoT.h

#include <Arduino.h>
#include <stdio.h>
// #include <esp_log.h>
#include "FS.h"
#include "SD.h"

#include "beeiot.h"     	// provides all GPIO PIN configurations of all sensor Ports !

//*******************************************************************
// ADS1115 I2C Library
//*******************************************************************
#include <Wire.h>
#include <Adafruit_ADS1015.h>
#include <driver/i2c.h>
#include "i2c_ads.h"
// i2c_config_t conf;          // ESP IDF I2C port configuration object

extern uint16_t	lflags;      // BeeIoT log flag field


Adafruit_ADS1115 ads(ADS_ADDR);    // Use this for the 16-bit version

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
int setup_i2c_ADS() {
// put your setup code here, to run once:

  // ADS1115S constructor
#ifdef  ADS_CONFIG
	BHLOG(LOGADS) Serial.println("  ADS: Init I2C-port incl. Alert line");
	pinMode(ADS_ALERT, INPUT);		// prepare Alert input line of connected ADS1511S at I2C Bus
	ads.begin();					// activate I2C driver based on std. I2C pins of ESp32 board.

   // change GAIN level if needed
   // ads.setGain(GAIN_TWOTHIRDS);  // see comments above

   // Optional: Setup 3V comparator on channel x
   // BHLOG(LOGADS) Serial.println("Comparator Threshold: 1000 (3.000V)");
   // ads.startComparator_SingleEnded(x, 1000);	// 1000 == 3.0V

   // 	BHLOG(LOGADS) i2c_scan();	// reqires I2C-CoreDriver running
#endif //ADS_CONFIG

  return 0;   // I2C-ADS device port is initialized
} // end of setup_i2c_ADS()


//***************************************************************
// I2C_scan()
// Test of I2C device Scanner
//***************************************************************
void i2c_scan(void) {

#ifdef ADS_CONFIG
	int i;
	esp_err_t espRc;

	printf("  ADS:      0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f\n");
	printf("  ADS: 00:         ");

	for (i=3; i< 0x78; i++) {
		i2c_cmd_handle_t cmd = i2c_cmd_link_create();
		i2c_master_start(cmd);
		i2c_master_write_byte(cmd, (i << 1) | I2C_MASTER_WRITE, 1 /* expect ack */);
		i2c_master_stop(cmd);

		espRc = i2c_master_cmd_begin(I2C_NUM_0, cmd, 10/portTICK_PERIOD_MS);
		if (i%16 == 0) {
			printf("\n  ADS: %.2x:", i);
		}
		if (espRc == 0) {
			printf(" %.2x", i);
		} else {
			printf(" --");
		}

		//ESP_LOGD(tag, "i=%d, rc=%d (0x%x)", i, espRc, espRc);
		i2c_cmd_link_delete(cmd);
	}
	printf("\n");
#endif // ADS_CONFIG
}

//***************************************************************
// ADS_read()
//  Test of ADS1115 I2C device
// 	IN:   channel = 0..3
//  OUT:  data  = voltage of adc line [mV]
// see also
//  https://esp32developer.com/programming-in-c-c/i2c-programming-in-c-c/using-the-i2c-interface
//
//***************************************************************
int16_t ads_read(int channel) {
#ifdef ADS_CONFIG
	int16_t adcdata;
	int16_t data;

	BHLOG(LOGADS) Serial.printf("\n  ADS: Single-ended read from AIN%i: ", channel);
	adcdata = ads.readADC_SingleEnded(channel);
	// ADC int. Reference: +/- 6.144V 
	// ADS1015: 11bit ADC -> 6144/2048  = 3 mV / bit
	// ADS1115: 15bit ADC -> 6144/32768 = 0.1875 mV / bit
	data = (int16_t)((float)adcdata * 0.1875);	// multiply by 1bit-sample in mV
	delay(100);

#endif // ADS_CONFIG    
	return data;
}

//***************************************************************
// ADS_COMPARE()
// Compare input channel with defined threshold in setup routine
// OUT:  data  = voltage of actual adc line
//***************************************************************
float ads_compare(/* channel was defined already in setup */) {
#ifdef ADS_CONFIG
int16_t adcdata;
float 	data;

  // Comparator will only de-assert after a read
  adcdata = ads.getLastConversionResults();
  BHLOG(LOGADS) Serial.print("  ADS: AINx (Comp.): "); Serial.println(adcdata);
  data = (float)((float)adcdata * 0.1875)/1000;	// devide by minimal sample step in Volt: 0,1875V

#endif // ADS_CONFIG
    
  return data;
}

//***************************************************************
// ADS_DIFF()
// Getting differential reading from AIN0 (P) and AIN1 (N)");
// ADC Range: +/- 6.144V (1 bit = 3mV/ADS1015, 0.1875mV/ADS1115)
// OUT:  results  = value of difference
//***************************************************************
int16_t ads_diff() {
#ifdef ADS_CONFIG
float   multiplier;
int16_t results;

  BHLOG(LOGADS) Serial.print("  ADS: Getting differential reading from AIN0 (P) and AIN1 (N)");
  
  /* Be sure to update this value based on the IC and the gain settings! */
  multiplier = 0.1875F; 	// ADS1115  @ +/- 6144 mV gain (16-bit results)

  results = ads.readADC_Differential_0_1();  
    
  BHLOG(LOGADS) {
   Serial.print("  ADS: Differential: ");
   Serial.print(results); 
   Serial.print("("); 
   Serial.print(results * multiplier); 
   Serial.println("mV)");
  }
#endif // ADS_CONFIG
    
  return results;
}