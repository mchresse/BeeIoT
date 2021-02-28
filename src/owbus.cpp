//*******************************************************************
// owbus.cpp
// from Project https://github.com/mchresse/BeeIoT
//
// Description:
// Contains main setup() and helper() routines for OneWire bus connected
// devices.
// See also at https://randomnerdtutorials.com/esp32-multiple-ds18b20-temperature-sensors/
//
//-------------------------------------------------------------------
// Copyright (c) 2019-present, Randolph Esser
// All rights reserved.
// This file is distributed under the BSD-3-Clause License
// The complete license agreement can be obtained at:
//     https://github.com/mchresse/BeeIoT/license
// For used 3rd party open source see also Readme_OpenSource.txt
//*******************************************************************
// This Module contains code derived from
// - OWBUS library example code
//*******************************************************************

//*******************************************************************
// OWBus Local Libraries
//*******************************************************************
// For ESP32-DevKitC PIN Configuration look at BeeIoT.h

#include <Arduino.h>
#include <stdio.h>
#include <esp_log.h>
#include "sdkconfig.h"
#include "FS.h"
#include "SD.h"

// DS18B20 libraries
#include <OneWire.h>
#include <DallasTemperature.h>

#include "beeiot.h"     // provides all GPIO PIN configurations of all sensor Ports !
#include "owbus.h"      // Provides OneWire bus device related settings

//***********************************************
// Global Sensor Data Objects
//***********************************************
extern dataset bhdb;                // central BeeIo DB
extern uint16_t	lflags;             // BeeIoT log flag field

// Setup a oneWire instance to communicate with a OneWire device
OneWire ds(ONE_WIRE_BUS);  // declare OneWire Bus Port with pullup

// Pass our oneWire reference to Dallas Temperature sensor
DallasTemperature OWsensors(&ds);
owbus_t	owdata;						// dataset of detected OW devices

bool ScanOwBus(int idx);
int printAddress(DeviceAddress deviceAddress);
int getDSType(byte DSType);

//*******************************************************************
// OW-bus Setup Routine
//*******************************************************************
int setup_owbus(int reentry) {
// put your setup code here, to run once:

// ONEWIRE Constructor
	owdata.numdev = 0;		// by now no dev. detected

#ifdef ONEWIRE_CONFIG
int idx=0;

	BHLOG(LOGOW) Serial.println("  OWBus: Init OneWire Bus");
	gpio_hold_dis(ONE_WIRE_BUS);
	OWsensors.begin();    // Init OW bus devices

	// scan OW Bus by addresses we found on the bus
	for(idx=0; idx < OW_MAXDEV; idx++){
		if(ScanOwBus(idx)){		// scan OW Bus and update owdata accordingly
			owdata.numdev++;
		}else{
			break;
		}
	}


    // locate devices on the bus
	// does not work with ESP32S & Dallas-lib v2.3.5 !  Delivers always =0
	// we can ignore this results here.
  	BHLOG(LOGOW) Serial.print("  OWBus: Get OW devices Count: ");
  	BHLOG(LOGOW) Serial.print(OWsensors.getDS18Count());
  	BHLOG(LOGOW) Serial.print(" of ");
  	BHLOG(LOGOW) Serial.print(OWsensors.getDeviceCount());
  	BHLOG(LOGOW) Serial.println(" devices found.");

	// just warn if ext. cfg. has changed; the code itself expects no pariste power
	if(OWsensors.isParasitePowerMode())
		BHLOG(LOGOW) Serial.print("  OWBus requests parasite power !\n");

  	BHLOG(LOGOW) Serial.printf("  OWBus: OW devices found (by Addr.): %i\n", owdata.numdev);

	if(owdata.numdev==0){
		return 0;	// no valid OW devices found
	}

	owdata.resolution = OWsensors.getResolution();
  	if(owdata.resolution != TEMPRESOLUTION){
	  // set the resolution to 12 bit per device
  		OWsensors.setResolution(TEMPRESOLUTION);
		owdata.resolution = OWsensors.getResolution();
	}

  	BHLOG(LOGOW) Serial.printf("  OWBUS: Used sensor resolution: %i\n", OWsensors.getResolution());

#endif // ONEWIRE_CONFIG

  return owdata.numdev;   // OneWire device dataset is initialized
} // end of setup_owbus()


//*******************************************************************
// OW-bus Scan Device Routine
// INPUT: Index of OW device
// OUTPUT: 	=false no more device detected
//			=true  new device detected -> owdata set updated
//*******************************************************************
bool ScanOwBus(int idx){
  	BHLOG(LOGOW) Serial.printf("    Device %i: S-Address: ", idx);
	if(! OWsensors.getAddress((uint8_t *) &owdata.dev[idx].sid, idx)){
		BHLOG(LOGOW) Serial.print("not found\n");
		owdata.dev[idx].type = -1;	// marker for invalid sensor data set
		return(false);
	}else{
  		owdata.dev[idx].type = printAddress(owdata.dev[idx].sid);	// show unique Sensor ID
  		BHLOG(LOGOW) Serial.println();
	}
	return(true);
} // end of ScanOwBus()



//*******************************************************************
// print DS Type string /wo new line depending on 1. Byte of DevAddr
//*******************************************************************
int getDSType(byte DSType){
  // the first ROM byte indicates which chip
  int type_s=0;
  switch (DSType) { // analyse 1. Byte of devAddr
    case 0x10:
      BHLOG(LOGOW)   Serial.print(" DS18S20");  // or old DS1820
      type_s = 1;
      break;
    case 0x28:
      BHLOG(LOGOW) Serial.print(" DS18B20");
      type_s = 0;
      break;
    case 0x22:
      BHLOG(LOGOW) Serial.print(" DS1822");
      type_s = 0;
      break;
    default:
      BHLOG(LOGOW) Serial.print("No DS18x20 family device.");
      return(type_s);
  }
  return(type_s);
}

// function to print a device address
int printAddress(DeviceAddress deviceAddress)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    // zero pad the address if necessary
    if (deviceAddress[i] < 16)
      BHLOG(LOGOW) Serial.print("0");
    BHLOG(LOGOW) Serial.print(deviceAddress[i], HEX);
  }
  BHLOG(LOGOW) Serial.print(" ");
  return(getDSType(deviceAddress[0]));
}

// read One Wire Sensor data based on given IDs (owbus.h)
// Using Dallas temperature device library
// Input: INdex of Datarow entry in bhdb
// Return:  Number of read devices

int GetOWsensor(int sample){
	if(owdata.numdev == 0)
		return(0);

#ifdef ONEWIRE_CONFIG
	BHLOG(LOGOW) Serial.println("  OWBus: Requesting temperatures...");

	OWsensors.setWaitForConversion(true);
	OWsensors.requestTemperatures(); // Send the command to get temperatures (all at once)

	if(owdata.dev[TEMP_Int].type >= 0){
  		bhdb.dlog[sample].TempIntern = OWsensors.getTempC(owdata.dev[TEMP_Int].sid);
	}else{
		bhdb.dlog[sample].TempIntern = -99;
	}

	if(owdata.dev[TEMP_Ext].type >= 0){
  		bhdb.dlog[sample].TempExtern = OWsensors.getTempC(owdata.dev[TEMP_Ext].sid);
	}else{
		bhdb.dlog[sample].TempExtern = -99;
	}

	if(owdata.dev[TEMP_BH].type >= 0){
  		bhdb.dlog[sample].TempHive = OWsensors.getTempC(owdata.dev[TEMP_BH].sid);
	}else{
		bhdb.dlog[sample].TempHive = -99;
	}
	BHLOG(LOGOW) Serial.printf("  OWBus: Int.Temp.Sensor (°C): %.2f\n", bhdb.dlog[sample].TempIntern);
	BHLOG(LOGOW) Serial.printf("  OWBus: Ext.Temp.Sensor (°C): %.2f\n", bhdb.dlog[sample].TempExtern);
	BHLOG(LOGOW) Serial.printf("  OWBus: HiveTemp.Sensor (°C): %.2f\n", bhdb.dlog[sample].TempHive);
#endif // ONEWIRE_CONFIG

	return(owdata.numdev);
} // end of GetOWsensor()

// end of owbus.cpp