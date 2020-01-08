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
#include "sdkconfig.h"

#include "beeiot.h" // provides all GPIO PIN configurations of all sensor Ports !

//*******************************************************************
// RTC DS3231 Libraries
//*******************************************************************
#include "RTClib.h"

//*******************************************************************
// Global Sensor Data Objects
//*******************************************************************
extern dataset bhdb;
extern int iswifi;    // WiFI semaphor
extern int isntp;     // by now we do not have any NTP client
int   isrtc =-1;      // =0 if RTC time discovered

extern uint16_t	lflags;      // BeeIoT log flag field

RTC_DS3231 rtc;     // Create RTC Instance

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
 
//*******************************************************************
// Setup_RTC(): Initial Setup of RTC module instance
//*******************************************************************
int setup_rtc () {
  isrtc =-1;

  if (! rtc.begin()) {
    Serial.println("  RTC: Couldn't find RTC device");
    return(isrtc);
  }
  isrtc = 0;  // RTC Module detected;
  //  if NTC based adjustment is needed use:
  //  static void adjust(const DateTime& dt);
  // or update by NTP: -> main() => ntp2rtc()
  
  rtc.writeSqwPinMode(DS3231_OFF);  // reset Square Pin Mode to 0Hz
  bhdb.dlog[bhdb.loopid].TempRTC = rtc.getTemperature();  // RTC module temperature in celsius degree
  BHLOG(LOGBH)  Serial.printf("  RTC: Temperature: %.2f °C, SqarePin switched off\n", bhdb.dlog[bhdb.loopid].TempRTC);

  if (rtc.lostPower()) {
    Serial.println("  RTC: lost power, check battery; lets set the time manually or by NTP !");
    // following line sets the RTC to the date &amp; time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date &amp; time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));

    isrtc =-2;  // power again but no valid time => lets hope for NTP update later on
  }
   return(isrtc);
} // end of rtc_setup()

//*******************************************************************
// NTP2RTC(): Update RTC by NTP time
//*******************************************************************
int ntp2rtc() {
struct tm tinfo;      // new time source: from NTP

  // if no RTC nor NTP Time at all, we give up.
  if(isntp != 0)   {
    BHLOG(LOGLAN) Serial.println("  NTP2RTC(-1): No NTP Server detected ");
    return(-1);       // is NTP server via Wifi active ?
  }
  if((isrtc != -2) && (isrtc != 0)) {  
    BHLOG(LOGLAN) Serial.println("  NTP2RTC(-2): No RTC Module detected ");
    return(-2);       // Need a valid RTC Module (even with powerloss status)
  }
  if(!getLocalTime(&tinfo)){ // get NTP server time
    Serial.print("  NTP2RTC(-3): Failed to obtain NTP time -> ");
    Serial.println("set time to build of sketch");
    
    // following line sets the RTC to the date &amp; time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // if we want to set it manually:
    // This line sets the RTC with an explicit date &amp; time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
    return(-3);       // no RTC nor NTP Time at all, we give up.
  }

  // Convert timeinfo to DateTime Struct:
  // DateTime (uint16_t year, uint8_t month, uint8_t day, uint8_t hour = 0, uint8_t min = 0, uint8_t sec = 0);
  rtc.adjust(DateTime(2000+tinfo.tm_year-100, tinfo.tm_mon+1, tinfo.tm_mday, tinfo.tm_hour, tinfo.tm_min, tinfo.tm_sec));

  DateTime dt = rtc.now();  // reread new time
  BHLOG(LOGLAN) Serial.print("  NTP2RTC: set new RTC Time: ");
  BHLOG(LOGLAN) Serial.println(String(dt.timestamp(DateTime::TIMESTAMP_FULL)));

  return(isrtc);
} // end of ntp2rtc()

//*******************************************************************
// getRTCtime(): read out current timeostamp and stor it to global BHDB
//*******************************************************************
int getRTCtime(){
// The formattedDate comes with the following format: 2018-05-28T16:00:13Z
//  We need to extract date and time
  DateTime dt = rtc.now();

  BHLOG(LOGLAN) Serial.print("  RTC: Get RTC Time: ");

  // using ISO 8601 Timestamp functions: YYYY-MM-DDTHH:MM:SS
  strncpy(bhdb.formattedDate, dt.timestamp(DateTime::TIMESTAMP_FULL).c_str(), LENFDATE);
  BHLOG(LOGLAN) Serial.print(bhdb.formattedDate);

  // Extract date: YYYY-MM-DD
  strncpy(bhdb.date, dt.timestamp(DateTime::TIMESTAMP_DATE).c_str(), LENDATE); 
  BHLOG(LOGLAN) Serial.print(" - ");
  BHLOG(LOGLAN) Serial.print(bhdb.date);
 
  // Extract time: HH:MM:SS
  sprintf(bhdb.time, dt.timestamp(DateTime::TIMESTAMP_TIME).c_str(), LENTIME); 
  BHLOG(LOGLAN) Serial.print(" - ");
  BHLOG(LOGLAN) Serial.println(bhdb.time);

  // last but not least: get current RTC temperature
  bhdb.dlog[bhdb.loopid].TempRTC = rtc.getTemperature();  // RTC module temperature in celsius degree
  return 0;
} // end of getRTCtime()


//*******************************************************************
// rtc_test(): Simple RTC test program.
//*******************************************************************
void rtc_test() {
  Serial.println("  RTC: STart RTC Test Output ...");
    DateTime now = rtc.now();
     
    Serial.print(now.year(), DEC);
    Serial.print('/');
    Serial.print(now.month(), DEC);
    Serial.print('/');
    Serial.print(now.day(), DEC);
    Serial.print(" (");
    Serial.print(daysOfTheWeek[now.dayOfTheWeek()]);
    Serial.print(") ");
    Serial.print(now.hour(), DEC);
    Serial.print(':');
    Serial.print(now.minute(), DEC);
    Serial.print(':');
    Serial.print(now.second(), DEC);
    Serial.println();
     
    Serial.print(" since midnight 1/1/1970 = ");
    Serial.print(now.unixtime());
    Serial.print("s = ");
    Serial.print(now.unixtime() / 86400L);
    Serial.println("d");
     
    // calculate a date which is 7 days and 30 seconds into the future
    DateTime future (now + TimeSpan(7,12,30,6));
     
    Serial.print(" now + 7d + 30s: ");
    Serial.print(future.year(), DEC);
    Serial.print('/');
    Serial.print(future.month(), DEC);
    Serial.print('/');
    Serial.print(future.day(), DEC);
    Serial.print(' ');
    Serial.print(future.hour(), DEC);
    Serial.print(':');
    Serial.print(future.minute(), DEC);
    Serial.print(':');
    Serial.print(future.second(), DEC);
    Serial.println();
     
    Serial.println();

} // end of rtc_test()