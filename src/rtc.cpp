//*******************************************************************
// rtc.cpp
// from Project https://github.com/mchresse/BeeIoT
//
// Description:
// RTC Setup() and useful function routines
// RTC DS3231 Module address: 0x68
// EEPROM AT24C32 address   : 0x53 (with Jumper on A2) (adjustable 0x50 ... 0x56)
// More Info: https://arduino-projekte.webnode.at/meine-libraries/rtc-ds3231/
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
// - RTClib library example code
//*******************************************************************

//*******************************************************************
// BeeIoT Libraries
//*******************************************************************

#include <Arduino.h>
#include <stdio.h>
#include <esp_log.h>
#include <time.h>
#include <driver/i2c.h>

#include "i2cdev.h"	// I2C master Port definitions

//*******************************************************************
// RTC DS3231 Libraries
//*******************************************************************
// #include "RTClib.h"
#include "ds3231.h"
#include "beeiot.h" // provides all GPIO PIN configurations of all sensor Ports !

//*******************************************************************
// Global Sensor Data Objects
//*******************************************************************
extern uint16_t	lflags;      // BeeIoT log flag field

extern dataset bhdb;			// central status DB
extern int isi2c;				// =0 no I2C master port claimed yet
extern i2c_port_t i2c_master_port;	// defined in i2cdev.cpp
extern int iswifi;  			// =1 WiFI network discovered
extern int isntp;   			// =1 NTP Server discovered

RTC_DATA_ATTR int   isrtc;   	// =1 RTC time discovered
i2c_dev_t i2crtc;				// Config settings of RTC I2C device

void setRTCtime(uint8_t yearoff, uint8_t month, uint8_t day,  uint8_t hour,  uint8_t min, uint8_t sec);

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};	// 0-6

//*******************************************************************
// Setup_RTC(): Initial Setup of RTC module instance
// Expects discovered I2C RC device by I2C_scan() -> isrtc=1
//*******************************************************************
int setup_rtc (int reentry) {
	float rtctemp;
	esp_err_t esprc;

	// expect I2C master port was scanned once before -> isrtc set
	if(!isi2c){	// I2C Master Port active + ADC detected ?
		// I2C should have been scanned by I2c_master_init() already
		// if i2c problems occurs -> should have been reported there
		if(!isrtc){
        	BHLOG(LOGADS) Serial.println("  RTC: No RTC DS3231 port detected");
			return(isrtc);
    	}
	}

	// setup RTC Device config set
    i2crtc.port = i2c_master_port;
    i2crtc.addr = RTC_ADDR;
    i2crtc.sda_io_num = I2C_SDA;
    i2crtc.scl_io_num = I2C_SCL;
    i2crtc.clk_speed = I2C_FREQ_HZ;

// For Wire-lib usage:
//  if (! rtc.begin()) {
//    Serial.println("  RTC: Couldn't find RTC device\n");
//    return(isrtc);
//  }
//  rtc.writeSqwPinMode(DS3231_OFF);  // reset Square Pin Mode to 0Hz

// For I2cdev-lib access test:
	esprc = ds3231_get_temp_float(&i2crtc, &rtctemp);
	if(esprc !=ESP_OK){
		isrtc =0;		// RTC does not react anyhow
        BHLOG(LOGADS) Serial.printf("  RTC: RD RTC DS3231 Temp.register failed (%i)\n", esprc);
		return(isrtc);
	}

	BHLOG(LOGADS) Serial.printf("  RTC: RTC DS3231 detected at port: 0x%02X\n", RTC_ADDR);
	bhdb.dlog[bhdb.loopid].TempRTC = rtctemp;  // RTC module temperature in celsius degree
	BHLOG(LOGBH)  Serial.printf("  RTC: Temperature: %.2f °C, SqarePin switched off\n", rtctemp);

  // if NTC based adjustment is needed use:  static void adjust(const DateTime& dt);
  // or update by NTP: -> main() => ntp2rtc() automatically
  // or update by JOIN-Cfg data from BIoT-GW time automatically

//  if (rtc.lostPower()) {
//    Serial.println("  RTC: lost power, check battery; lets set the time manually or by NTP !");
    // following line sets the RTC to the date &amp; time this sketch was compiled
//    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date &amp; time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
//    isrtc =0;  // power again but no valid time => lets hope for NTP update later on
//  }
	return(isrtc);

} // end of rtc_setup()

//*******************************************************************
// NTP2RTC(): Update RTC by NTP time
//*******************************************************************
int ntp2rtc() {
struct tm tinfo;      // new time source: from NTP

  // if no RTC nor NTP Time at all, we give up.
  if(!isntp)   {
    BHLOG(LOGLAN) Serial.println("  NTP2RTC(-1): No NTP Server detected ");
    return(-1);       // is NTP server via Wifi active ?
  }
  if(!isrtc) {
    BHLOG(LOGLAN) Serial.println("  NTP2RTC(-2): No RTC Module detected ");
    return(-2);       // Need a valid RTC Module (even with powerloss status)
  }
  if(!getLocalTime(&tinfo)){ // get NTP server time
    Serial.print("  NTP2RTC(-3): Failed to obtain NTP time -> ");
    Serial.println("set time to build of sketch");

    // if we want to set it manually:
    // This line sets the RTC with an explicit date &amp; time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
    return(-3);       // no RTC nor NTP Time at all, we give up.
  }

    BHLOG(LOGLAN) Serial.println(&tinfo, "  NTP2RTC: by NTP-Time: %A, %B  %Y-%m-%dT%H:%M:%S");
	ds3231_set_time(&i2crtc, &tinfo);


  return(isrtc);
} // end of ntp2rtc()


//*******************************************************************
// setRTCtime(): set current time&date of RTC
// INPUT:
//    yearoff 2 digits (offset to year 1900)
//    month 0-11
//    day   1-31
//    hour  0-23
//    min   0-59
//    sec   0-59
//*******************************************************************
void setRTCtime(uint8_t yearoff, uint8_t month, uint8_t day, uint8_t hour = 0, uint8_t min = 0, uint8_t sec = 0){
	struct tm tinfo;

	if(isrtc){
		tinfo.tm_year	= yearoff;		// need offset to 1900  e.g.  2020 -> 120
		tinfo.tm_mon	= month;		// 0-11
		tinfo.tm_mday	= day;			// 1-31
		tinfo.tm_hour	= hour;			// 0-23
		tinfo.tm_min	= min;			// 0-59
		tinfo.tm_sec	= sec;			// 0-59
		tinfo.tm_wday	= 0;			// 0-6 (0=SU)
		ds3231_set_time(&i2crtc, &tinfo);

		// reread new time: DS3231 delivers time in struct TM format:
		ds3231_get_time(&i2crtc, &tinfo);
   		BHLOG(LOGLAN) Serial.println(&tinfo, "  SetRTC: new-RTC Time: %Y-%m-%dT%H:%M:%S");
	}
}


//*******************************************************************
// getRTCtime(): read out current timeostamp and store it to global BHDB
//*******************************************************************
int getRTCtime(void){
	float rtctemp;
	struct tm * tinfo;

	// DS3231 delivers time in struct TM format:
	ds3231_get_time(&i2crtc, & bhdb.stime);

	tinfo = & bhdb.stime;			// get structure time ptr.
	bhdb.rtime = mktime(tinfo);		// save RTC time as raw time value

	// Convert timeinfo to DateTime Struct:
	// DateTime (uint16_t year, uint8_t month, uint8_t day, uint8_t hour = 0, uint8_t min = 0, uint8_t sec = 0);
    // (tinfo.tm_year-100, tinfo.tm_mon+1, tinfo.tm_mday,
	//  tinfo.tm_hour, tinfo.tm_min, tinfo.tm_sec);

	// The formattedDate comes with the following format: 2018-05-28T16:00:13Z
	//  We need to extract date and time

	BHLOG(LOGLAN) Serial.print("  RTC: Get RTC Time: ");
	// Get date and time; 2019-10-28T08:09:47Z
	// using ISO 8601 Timestamp functions: YYYY-MM-DDTHH:MM:SS
	sprintf(bhdb.formattedDate,"%i-%02i-%02iT%02i:%02i:%02i", 	tinfo->tm_year+1900, tinfo->tm_mon+1, tinfo->tm_mday, tinfo->tm_hour, tinfo->tm_min, tinfo->tm_sec);
	BHLOG(LOGLAN) Serial.print(bhdb.formattedDate);
	BHLOG(LOGLAN) Serial.print(ctime(&bhdb.rtime));	// test print of time_t value as string

	// Extract date: YYYY-MM-DD
	sprintf(bhdb.date,"%i-%02i-%02i", tinfo->tm_year+1900, tinfo->tm_mon+1, tinfo->tm_mday);
	BHLOG(LOGLAN) Serial.print(" - ");
	BHLOG(LOGLAN) Serial.print(bhdb.date);

	// Extract time: HH:MM:SS
	sprintf(bhdb.time,"%02i:%02i:%02i ", tinfo->tm_hour, tinfo->tm_min, tinfo->tm_sec);
	BHLOG(LOGLAN) Serial.print(" - ");
	BHLOG(LOGLAN) Serial.print(bhdb.time);

    BHLOG(LOGLAN) Serial.print(daysOfTheWeek[tinfo->tm_wday]);

	// last but not least: get current RTC temperature
	ds3231_get_temp_float(&i2crtc, &rtctemp);
	bhdb.dlog[bhdb.loopid].TempRTC = rtctemp;  // RTC module temperature in celsius degree
	BHLOG(LOGLAN)  Serial.printf(" RTC-Temp.: %.2f °C\n", rtctemp);

  return 0;
} // end of getRTCtime()


//*******************************************************************
// rtc_test(): Simple RTC test program.
//*******************************************************************
void rtc_test() {
	struct tm tinfo;

	// DS3231 delivers time in struct TM format:
	ds3231_get_time(&i2crtc, &tinfo);

	BHLOG(LOGLAN) Serial.println(&tinfo, "  RTC: Test Output:  %A, %B %d %Y %H:%M:%S");

} // end of rtc_test()