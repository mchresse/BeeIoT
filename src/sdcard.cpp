//*******************************************************************
// SDCard.cpp  
// from Project https://github.com/mchresse/BeeIoT
//
// Description:
// Contains main setup() and loop() routines for SPi based MM
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
// - 
//*******************************************************************
// For ESP32-DevKitC PIN Configuration look at BeeIoT.h

#include <Arduino.h>
#include <stdio.h>
#include <esp_log.h>
#include "sdkconfig.h"

//*******************************************************************
// SDCard SPI Library
//*******************************************************************

// Libraries for SD card
#include "FS.h"
#include "SD.h"
#include "SPI.h"

#include "beeiot.h"     // provides all GPIO PIN configurations of all sensor Ports !
#include "sdcard.h"     // detailed SD parameters

extern int issdcard;    // =0 SDCard found
extern uint16_t	lflags;      // BeeIoT log flag field


//*******************************************************************
// SDCard SPI Setup Routine
//*******************************************************************
int setup_sd() {  // My SDCard Constructor
#ifdef SD_CONFIG
  BHLOG(LOGSD) Serial.println("  SD: SD Card");

  uint8_t cardType = SD.cardType();
  if(cardType == CARD_NONE) {
    issdcard = -2;
    Serial.println("  SD: No SD card found");
    return issdcard;
  }
  
  BHLOG(LOGSD) Serial.print("  SD: SD Card Type: ");
  if (cardType == CARD_MMC) {
    BHLOG(LOGSD) Serial.println("MMC");
  } else if (cardType == CARD_SD) {
    BHLOG(LOGSD) Serial.println("SDSC");
  } else if (cardType == CARD_SDHC) {
    BHLOG(LOGSD) Serial.println("SDHC");
  } else {
    BHLOG(LOGSD) Serial.println("UNKNOWN");
  }

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  BHLOG(LOGSD) Serial.printf("  SD: SD Card Size: %lluMB\n", cardSize);
  issdcard = 0;   // we have an SDCard

  if(lflags & LOGSD){   // lets get some Debug data from SDCard
    listDir(SD, "/", 3);            // list 3 directory levels from Root
    // readFile(SD, SDLOGPATH); 
    // deleteFile(SD, SDLOGPATH);   // for test purpose only
  }

  // If the SDLOGPATH file doesn't exist
  // Create a new file on the SD card and write the data label header line
  File file = SD.open(SDLOGPATH);
  if(!file) {
    BHLOG(LOGSD) Serial.printf("  SD: SD:Logfile %s doesn't exist -> Creating new file + header...", SDLOGPATH);
    writeFile(SD, SDLOGPATH, "Sample-ID, Date, Time, BeeHiveWeight, TempExtern, TempIntern, TempHive, TempRTC, ESP3V, Board5V, BattCharge, BattLoad, BattLevel\r\n");
  } else {
    BHLOG(LOGSD) Serial.printf("  SD: SD:Logfile: %s not found\n", SDLOGPATH);  
  }
  file.close();

#endif // SD_CONFIG

 return issdcard;   // SD card port is initialized
} // end of setup_spi_sd()

// end of sdcard.cpp