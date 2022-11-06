//*******************************************************************
// HX711Scale.cpp
// from Project https://github.com/mchresse/BeeIoT
//
// Description:
// Contains main setup() and loop() routines for HX711 connected to
// a weight scale module.
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
// For ESP32-DevKitC to HX711 PIN Configuration look at BeeIoT.h

//*******************************************************************
// HX711Scale Local Libraries
//*******************************************************************

#include <Arduino.h>
#include <stdio.h>
#include <esp_log.h>
#include "sdkconfig.h"

// Library for generic HX711 Access
#include <HX711.h>
#include "HX711Scale.h" // Provides HX711 weight scale related settings

#include "beeiot.h"     // provides all GPIO PIN configurations of all sensor Ports !

//*******************************************************************
// Global Sensor Data Object
//*******************************************************************
extern uint16_t	lflags; // BeeIoT log flag field
extern dataset	bhdb;
int isscale=0;
HX711 scale;            // the one and only weight cell

//*******************************************************************
// HX711Scale Setup Routine
//*******************************************************************
int setup_hx711Scale(int reentry) {
// put your setup code here, to run once:
#ifdef HX711_CONFIG
float	scale_offs;

// HX711 constructor:
	BHLOG(LOGHX) Serial.println("  HX711: init Weight cell ADC port");
	gpio_hold_dis(HX711_SCK);             // enable HX711_SCK for Dig.-IO
	gpio_hold_dis(HX711_DT);              // enable HX711_DT for Dig.-IO

	// Actiovate Scale ADC IO Port
	scale.begin(HX711_DT, HX711_SCK,128);// declare GPIO pin connection + gain factor: A128

	if(scale.wait_ready_timeout() == false){
		isscale=0;
		return(0);	// no weight scale device ADC connected
	}


	scale.set_scale(scale_DIVIDER);       // define unit value per kg

	BHLOG(LOGHX) Serial.printf("  HX711: Weight Cfg.-Calib: %.3f kg\n", (float) bhdb.woffset/(-1000));
	scale_offs = (float) scale_DIVIDER * ((float) bhdb.woffset/(-1000));	// get tare cfg. value as float
	scale.set_offset((long)scale_offs);	// define base value for 0kg
					// (e.g. reflects weight of weight cell cover board)

	BHLOG(LOGHX) Serial.print("  HX711: Offset(raw): ");
	BHLOG(LOGHX) Serial.print(scale_offs, 10);
	BHLOG(LOGHX) Serial.print(" - Unit(raw): ");
	BHLOG(LOGHX) Serial.print(scale_DIVIDER, 10);
	BHLOG(LOGHX) Serial.println(" per kg");

	scale.power_down();
	isscale=1;
	#endif // HX711_CONFIG

  return isscale;   // HX711 & wight cell initialized
} // end of setup_HX711Scale()


//*******************************************************************
// BeeIoT HX711 - Weight Scale Read Routine
// Activates HX711 device and Scans load cell value
// Input: Mode
//    =0  Read aw value
//    =1  Read raw value adopted by set_scale values
// return: requested value by mode
//*******************************************************************
float HX711_read(int mode){
float reading = 0;

#ifdef HX711_CONFIG
  if (mode == 0) {
      reading = scale.get_value(5);     // get a raw value as average of 5 reads
  } else {
   // Acquire reading without blocking
   if (scale.wait_ready_timeout(1000)) {    // wait some time till bits are more stable....
     reading = scale.get_units(5);          // get a value as average of 5 reads corrected by offset and unit value
                                            // result is a kg unit value
   } else {
     Serial.println("  HX711: no weight cell not found.");
      reading = 0;
   }
  }
#endif // HX711_CONFIG

  return (reading);     // return raw value or unit value depending on the selected mode
 } // end of HX711_read()

// end of HX71Scale.cpp
