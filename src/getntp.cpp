//*******************************************************************
// getntp.cpp
// from Project https://github.com/mchresse/BeeIoT
//
// Description:
// NTP - Time Support routines
// Contains main setup() and helper() routines for NTP
// Requires WIFI connected !
//
//----------------------------------------------------------
// Copyright (c) 2019-present, Randolph Esser
// All rights reserved.
// This file is distributed under the BSD-3-Clause License
// The complete license agreement can be obtained at:
//     https://github.com/mchresse/BeeIoT/license
// For used 3rd party open source see also Readme_OpenSource.txt
//*******************************************************************
//
// This Module contains code derived from
// - Espressif Wifi.h library example code
//*******************************************************************

#include <Arduino.h>
#include <stdio.h>
#include <esp_log.h>
#include "sdkconfig.h"
//#include "FS.h"
//#include "SD.h"

//*******************************************************************
// GETNTP - Local Libraries
//*******************************************************************
// Libraries for WIFI & to get time from NTP Server
#include "time.h"

#include "beeiot.h"     // provides all GPIO PIN configurations of all sensor Ports !

//*******************************************************************
// Global Sensor Data Objects
//*******************************************************************
extern dataset bhdb;
extern int isrtc;     // RTC semaphor

extern uint16_t	lflags;      // BeeIoT log flag field


//*******************************************************************
// Local functions and parameters
//*******************************************************************
// const char* ntpServer = "de.pool.ntp.org";
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 3600;
#define TZ_INFO "WEST-1DWEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00" // Western European Time



//*******************************************************************



//*******************************************************************
// getTimeStamp()
// 1. check for RTC: obtain RTC time and update BHDB
// 2. check for NTP source -> obtain NTP time and update BHDB
//  Return
//     0: update RTC time to BHDB
//    -1: NTP access failed
//    -2: RTC & NTP failed -> no update done
//*******************************************************************
int getTimeStamp() {
  struct tm * tinfo;
  char tmstring[30];

	if(isrtc){   // do we have RTC time resource
		// we prefer local time from RTC if available
		getRTCtime();  // update global BHDB by RTC date & time
		BHLOG(LOGLAN) Serial.println("  RTC: BHDB updated by RTC time");
	}else{
		sprintf(bhdb.formattedDate, "YYYY\\MM\\DDTHH:MM:SSZ");
		sprintf(bhdb.date,      "YYYY\\MM\\DD");
		sprintf(bhdb.time,      "HH:MM:SS");
		bhdb.rtime = 0;			// set default: 00:00 hours, Jan 1, 1970 UTC
		bhdb.stime.tm_hour = 0;
		bhdb.stime.tm_min = 0;
		bhdb.stime.tm_sec = 0;
		bhdb.stime.tm_mon = 0;		// 0..11 -> 1.January.1970
		bhdb.stime.tm_mday = 1;		// 1..31
		bhdb.stime.tm_year = 70;	// years since 1900
		return(-2);               // we give up - no time source available
	}
  return(0);
} // end of getTimeStamp()

//*******************************************************************
// NTP print current local Time Routine
//*******************************************************************
void printLocalTime() {
  struct tm tinfo;
  if(!getLocalTime(&tinfo)){
    BHLOG(LOGLAN) Serial.println("  RTC: Failed to obtain time");
    return;
  }
  asctime(&tinfo);
//  Serial.println(&tinfo, "%A, %B %d %Y %H:%M:%S");
}
