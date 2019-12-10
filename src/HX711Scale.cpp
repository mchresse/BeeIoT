/******************************************************************************* 
 * File:   HX711Scale.cpp - HX711 weight scale Support routines
 * Author: Randolph Esser - Copyright 2019
 * Created on 1. October 2019
 * This file is part of the "BeeIoT" program/project.
 *
 * Description:
 * Contains main setup() and loop() routines for HX711 connected to
 * a weight scale module.
 *******************************************************************************
 * For ESP32-DevKitC to HX711 PIN Configuration look at BeeIoT.h
 */ 

//*******************************************************************
// HX711Scale Local Libraries
//*******************************************************************

#include <Arduino.h>
#include <stdio.h>
#include <esp_log.h>
#include "sdkconfig.h"
#include "FS.h"
#include "SD.h"

// Library for generic HX711 Access
#include <HX711.h>
#include "HX711Scale.h" // Provides HX711 weight scale related settings

#include "beeiot.h"     // provides all GPIO PIN configurations of all sensor Ports !

//*******************************************************************
// Global Sensor Data Object
//*******************************************************************
extern uint16_t	lflags;      // BeeIoT log flag field

HX711 scale;    

//*******************************************************************
// HX711Scale Setup Routine
//*******************************************************************
int setup_hx711Scale() {
// put your setup code here, to run once:

#ifdef HX711_CONFIG
// HX711 constructor:
  BHLOG(LOGHX) Serial.println("  Setup: HX711 Weight cell ADC port");
  scale.begin(HX711_DT, HX711_SCK);     // declare GPIO pin connection
  scale.set_scale(scale_DIVIDER);       // define unit value per kg
  scale.set_offset(scale_OFFSET);       // define base value for 0kg
                  // (e.g. reflects weight of weight cell cover board)
  BHLOG(LOGHX) Serial.print("  Setup: HX711-Offset(raw): "); 
  BHLOG(LOGHX) Serial.print(scale_OFFSET, 10);
  BHLOG(LOGHX) Serial.print(" - Unit(raw): ");
  BHLOG(LOGHX) Serial.print(scale_DIVIDER, 10);
  BHLOG(LOGHX) Serial.println(" per kg");


  // Following lines corresponds to HX711_ADC lib only.
  // HX711 constructor (dout pin, sck pin)
  // HX711_ADC scale.(HX711_DT, HX711_SCK);
#endif // HX711_CONFIG

  return 0;   // HX711 & wight cell initialized
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
     Serial.println("  HX711 not found.");
      reading = 0;
   }
  } 
#endif // HX711_CONFIG
  return (reading);     // return raw value or unit value depending on the selected mode
 } // end of HX711_read()

// end of HX71Scale.cpp
