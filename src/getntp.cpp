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
#include <WiFi.h>
#include "time.h"
// #include <NTPClient.h>
// #include <WiFiUdp.h>

#include "beeiot.h"     // provides all GPIO PIN configurations of all sensor Ports !

//*******************************************************************
// Global Sensor Data Objects
//*******************************************************************
extern dataset bhdb;
extern int iswifi;    // WiFI semaphor
extern int isrtc;     // RTC semaphor
       int isntp = 0; // by now we do not have any NTP client

extern uint16_t	lflags;      // BeeIoT log flag field


//*******************************************************************
// Local functions and parameters
//*******************************************************************
const char* ntpServer = "de.pool.ntp.org";
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 3600;
#define TZ_INFO "WEST-1DWEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00" // Western European Time



//*******************************************************************
//*******************************************************************
// NTP Setup Routine
//*******************************************************************
int setup_ntp(int reentry) { // NTP Constructor
isntp = 0;
#ifdef NTP_CONFIG
  if(!reentry){
	BHLOG(LOGLAN) Serial.println("  Setup: Init NTP Client");
		/*
		// Initialize a NTPClient to get time
		timeClient.begin();
		// Set offset time in seconds to adjust for your timezone, for example:
		// GMT +1 = 3600    // for MET
		// GMT +8 = 28800
		// GMT -1 = -3600
		// GMT 0 = 0
		timeClient.setTimeOffset(gmtOffset_sec);
		*/
		//init and get the time
		//  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
		configTzTime(TZ_INFO, ntpServer);
		isntp = 1;      // now we are up to date


	  BHLOG(LOGBH) printLocalTime();
  }
#endif // NTP_CONFIG

  return isntp;   // NTP Source prepared
} // end of setup_ntp()


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
  struct tm timeinfo;
  char tmstring[30];

	if(isrtc){   // do we have RTC time resource
		// we prefer local time from RTC if available
		getRTCtime();  // update global BHDB by RTC date & time
		BHLOG(LOGLAN) Serial.println("  NTP: BHDB updated by RTC time");
		return(0);
	}

	if(!isntp){		// no RTC nor NTP Time at all
		sprintf(bhdb.formattedDate, "YYYY\\MM\\DDTHH:MM:SSZ");
		sprintf(bhdb.date,      "YYYY\\MM\\DD");
		sprintf(bhdb.time,      "HH:MM:SS");
		return(-2);               // we give up
	}
	
	// alternatively we search for NTP server via Wifi
	if(!getLocalTime(&timeinfo)){
      BHLOG(LOGLAN) Serial.println("  NTP: Failed to obtain NTP time");
      isntp = 0;				// remember NTP access failed
      return(-1);             	// no RTC nor NTP Time at all, we give up.
    }
 
// For NTP: We need to extract date and time from timeinfo (struct tm)
// The formattedDate should have the following format: 2018-05-28T16:00:13Z
// sprintf(tmstring, "%4i\%2i\%2iT%2i:%2i:%2iZ", 
//                    1900+timeinfo.tm_year,
//                    timeinfo.tm_mon,
//                    timeinfo.tm_mday,
//                    timeinfo.tm_hour,
//                    timeinfo.tm_min,
//                    timeinfo.tm_sec);
  strftime(tmstring, 30, "%Y\%m\%dT%H:%M:%SZ", &timeinfo);
  strncpy(bhdb.formattedDate, tmstring, LENFDATE);
  BHLOG(LOGLAN) Serial.print("    ");
  BHLOG(LOGLAN) Serial.print(bhdb.formattedDate);

  // Extract date
  strftime(tmstring, 30, "%Y\%m\%d", &timeinfo);
  strncpy(bhdb.date, tmstring, LENDATE);
  BHLOG(LOGLAN) Serial.print(" - ");
  BHLOG(LOGLAN) Serial.print(bhdb.date);
 
  // Extract time
  strftime(tmstring, 30, "%H:%M:%S", &timeinfo);
  strncpy(bhdb.time, tmstring, LENTIME);
  BHLOG(LOGLAN) Serial.print(" - ");
  BHLOG(LOGLAN) Serial.println(bhdb.time);

  return(0);
} // end of getTimeStamp()

//*******************************************************************
// NTP print current local Time Routine
//*******************************************************************
void printLocalTime() {
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    BHLOG(LOGLAN) Serial.println("  NTP: Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}
