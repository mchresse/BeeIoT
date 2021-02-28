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

#define OW_MAXDEV  4
#define ONE_WIRE_RETRY	3

// Physical ID of each DS18B20 OW Device
#define TEMP_0   { 0x28, 0x20, 0x1F, 0x8E, 0x1F, 0x13, 0x01, 0x85}    // -> Internal Weight cell temperature
#define TEMP_1   { 0x28, 0xAA, 0xE4, 0x6D, 0x18, 0x13, 0x02, 0x2F}    // -> BeeHive internal temperature
#define TEMP_2   { 0x28, 0xAA, 0xB3, 0xF2, 0x52, 0x14, 0x01, 0x47}    // -> External temperature
// #define TEMP_2   { 0x28, 0xAA, 0xCA, 0x6A, 0x18, 0x13, 0x2, 0xF3}  // old sensor
#define TEMPRESOLUTION 12

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