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
// Return: number of detected OW devices
//*******************************************************************
int setup_owbus(int reentry) {
	// ONEWIRE Constructor
	owdata.numdev = 0;		// by now no dev. detected

#ifdef ONEWIRE_CONFIG

	BHLOG(LOGOW) Serial.println("  OWBus: Init OneWire Bus");
	//	gpio_hold_dis(ONE_WIRE_BUS);
	OWsensors.begin();    // Init OW bus devices
	// Grab a count of devices on the wire

// does not work for ESP32 implementation
//  int numberOfDevices = OWsensors.getDeviceCount();
// 	BHLOG(LOGOW) Serial.printf("  OWBus: Lib Device count: %i\n", numberOfDevices);

	OWsensors.setResolution(TEMPRESOLUTION);		// set Sample Bit resolution
	owdata.resolution = OWsensors.getResolution();	// save configured resolution

    // locate devices on the bus
	// scan OW Bus by # of expected devices on the bus
	if(!ScanOwBus(OW_MAXDEV)){			// scan OW Bus and update owdata accordingly
		return(owdata.numdev);			// expected # of sensors not found
	}

	// just warn if external configuration has changed (Powerline broken);
	// The code itself expects no pariste power
	if(OWsensors.isParasitePowerMode()){
		BHLOG(LOGOW) Serial.print("  OWBus requests parasite power !\n");
	}


  	BHLOG(LOGOW) Serial.printf("  OWBus: OW sensors found: %i, Resolution: %ibit\n",
	  			owdata.numdev, OWsensors.getResolution());

#endif // ONEWIRE_CONFIG

  return (owdata.numdev);   // OneWire device dataset is initialized
} // end of setup_owbus()


//*******************************************************************
// OW-bus Scan Device Routine
// INPUT: Max-# of expected OW devices
// OUTPUT: 	=false no more device detected
//			=true  new device detected -> owdata set updated
//*******************************************************************
bool ScanOwBus(int maxidx){
int idx;
	owdata.numdev=0;	// reset number of detected sensors;
	for(idx=0; idx < maxidx; idx++){
		BHLOG(LOGOW) Serial.printf("    Sensor-%i: UID: ", idx);
		if(! OWsensors.getAddress(owdata.dev[idx].sid, idx)){
			BHLOG(LOGOW) Serial.print("not found\n");
			owdata.dev[idx].type = -1;	// marker for invalid sensor data set
			return(false);
		}else{
			owdata.dev[idx].type = printAddress(owdata.dev[idx].sid);	// show unique Sensor ID
			BHLOG(LOGOW) Serial.println();
			owdata.numdev++;
		}
	}
	return(true);
} // end of ScanOwBus()



//*******************************************************************
// print DS Type string /wo new line depending on 1. Byte of DevAddr
// Input:	DeviceTypeID from DeviceAddress[0]
// Return:	Device Type classifier
//		1: DS18S20
//		2: DS18B20
//		3: DS1822
//<other>: unknown type ID
//*******************************************************************
int getDSType(byte DSType){
  // the first ROM byte indicates which chip
  int type_s=0;

  switch (DSType) { // analyse 1. Byte of devAddr
    case 0x10:
      BHLOG(LOGOW) Serial.print(" DS18S20");  // or old DS1820
      type_s = 1;
      break;
    case 0x28:
      BHLOG(LOGOW) Serial.print(" DS18B20");
      type_s = 2;
      break;
    case 0x22:
      BHLOG(LOGOW) Serial.print(" DS1822");
      type_s = 3;
      break;
    default:
      BHLOG(LOGOW) Serial.print("No DS18x20 family device.");
  }
  return(type_s);
}

//*******************************************************************
// function to print a OW device address of 8 Byte Bin array to ASCII
// Input: 	OW-Device Address (8 Byte)
// Output:  DeviceType classifier from getDSType()
//*******************************************************************
int printAddress(DeviceAddress deviceAddress)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    // zero pad the address if necessary
    if (deviceAddress[i] < 16){
      	BHLOG(LOGOW) Serial.print("0");
	}
    BHLOG(LOGOW) Serial.print(deviceAddress[i], HEX);
  }
  BHLOG(LOGOW) Serial.print(" ");
  return(getDSType(deviceAddress[0]));
}

//*******************************************************************
// read One Wire Sensor data based on given IDs (owbus.h)
// Using Dallas temperature device library
// Input: INdex of Datarow entry index in bhdb
// Return:  Number of read devices
// 			bhdb.dlog[sample].Tempxxxx = Temp value
//										= -99C -> device not scanned
//										= -98C -> scanned nbut disconnected
//*******************************************************************
int GetOWsensor(void){
//	if(owdata.numdev < OW_MAXDEV)	// no sufficient number of OW devices detected
//		return(0);					// OWBus broken

#ifdef ONEWIRE_CONFIG
	float value;	// temperature value for evaluation
	int   datacount=0;

	BHLOG(LOGOW) Serial.println("  OWBus: Requesting temperatures...");

	// Send the command to prepare temperatures (all at once)
	// inclusive wait time till conversion has finished: 9= 94ms, 10=188ms, 11=375ms, 12=750ms
	OWsensors.requestTemperatures();

	// check for a valid scanned device first
	if((owdata.dev[TEMP_Int].type >= 0) & (owdata.numdev > TEMP_Int)){
  		value = OWsensors.getTempC(owdata.dev[TEMP_Int].sid);
		if((value == 85.0) || (value == DEVICE_DISCONNECTED_C)){	// error occured with reading
			value = -98.0; // reinterpret: device scanned but disconnected
		}else{
			datacount++;
		}
	}else{
		value = -99.0;	// device not scanned before
	}
	bhdb.dlog.TempIntern = value;		// save result to BHDB
	BHLOG(LOGOW) Serial.printf("  OWBus[%d]: Int.Temp.Sensor (°C): %.2f\n", TEMP_Int, value);

	if((owdata.dev[TEMP_Ext].type >= 0) & (owdata.numdev > TEMP_Ext)){
	  	value = OWsensors.getTempC(owdata.dev[TEMP_Ext].sid);
		if((value == 85.0) || (value == DEVICE_DISCONNECTED_C)){	// error occured with reading
			value = -98.0; // reinterpret: device scanned but disconnected
		}else{
			datacount++;
		}
	}else{
		value = -99.0;	// device not scanned before
	}
	bhdb.dlog.TempExtern = value;		// save result to BHDB
	BHLOG(LOGOW) Serial.printf("  OWBus[%d]: Ext.Temp.Sensor (°C): %.2f\n", TEMP_Ext,value);

	if((owdata.dev[TEMP_BH].type >= 0) & (owdata.numdev > TEMP_BH)){
	  	value = OWsensors.getTempC(owdata.dev[TEMP_BH].sid);
		if((value == 85.0) || (value == DEVICE_DISCONNECTED_C)){	// error occured with reading
			value = -98.0; // reinterpret: device scanned but disconnected
		}else{
			datacount++;
		}
	}else{
		value = -99.0;	// device not scanned before
	}
	bhdb.dlog.TempHive = value;		// save result to BHDB
	BHLOG(LOGOW) Serial.printf("  OWBus[%d]: HiveTemp.Sensor (°C): %.2f\n", TEMP_BH, value);

#endif // ONEWIRE_CONFIG

	return(owdata.numdev);
} // end of GetOWsensor()

// end of owbus.cpp