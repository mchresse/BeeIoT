//*******************************************************************
// owbus.h
// from Project https://github.com/mchresse/BeeIoT
//
// Description:
// OneWire connected device parameter file
//
//----------------------------------------------------------
// Copyright (c) 2019-present, Randolph Esser
// All rights reserved.
// This file is distributed under the BSD-3-Clause License
// The complete license agreement can be obtained at:
//     https://github.com/mchresse/BeeIoT/license
// For used 3rd party open source see also Readme_OpenSource.txt
//*******************************************************************

#ifndef OWBUS_H
#define OWBUS_H

#define OW_MAXDEV  		3		// #of expected OW devices
#define ONE_WIRE_RETRY	2		// number of retries if value is -99C

// Physical ID of each DS18B20 OW Device
// => obsolete: devices are detcted dynamically for easier replacement in case of a defect
// #define TEMP_0   { 0x28, 0x20, 0x1F, 0x8E, 0x1F, 0x13, 0x01, 0x85}    // -> Internal Weight cell temperature
// #define TEMP_1   { 0x28, 0xAA, 0xCA, 0x6A, 0x18, 0x13, 0x02, 0xF3}    // -> External temperature
// #define TEMP_2   { 0x28, 0xAA, 0xE4, 0x6D, 0x18, 0x13, 0x02, 0x2F}    // -> BeeHive internal temperature

// No. Bits	Max Conv.Time	Temp.-Resolution	Bits to Ignore	Mask
// 		9		93.75ms			0.500°C			2,1,0			0x0FF8
// 		10		187.5ms			0.250°C			1,0				0x0FFC
// 		11		375ms		  	0.125°C			0				0x0FFE
// 		12		750ms			0.0625°C		-				0x0FFF
#define TEMPRESOLUTION	10

enum stype {	// sensor index to type assignment
	TEMP_Int=0,	// Internal TempSensor for Weight cell calibration
	TEMP_Ext,	// External TempSensor
	TEMP_BH,	// BeeHive TempSensor
	SENSOR_A,	// dummy placeholder
};

typedef struct {
	DeviceAddress sid;	// serial unique ID of OW device
	int type;				// sensor type eg. DS18B20, ..., =-1: no sensor
} owdev_t;


// data set holdeing data of discovered OW devices
typedef struct {
	int numdev;			// # of detected devices
	int	resolution;		// all sensor resolution: 9,10,11,12 bit
	owdev_t dev[OW_MAXDEV];
} owbus_t;

#endif // end of owbus.h