//*******************************************************************
// multiSPI.cpp  
// from Project https://github.com/mchresse/BeeIoT
//
// Description:
// Setup of V-SPI Bus for multile clients
// Majorly defines pinmode of Std. SPI IO lines
//
//-------------------------------------------------------------------
// Copyright (c) 2019-present, Randolph Esser
// All rights reserved.
// This file is distributed under the BSD-3-Clause License
// The complete license agreement can be obtained at: 
//     https://github.com/mchresse/BeeIoT/license
// For used 3rd party open source see also Readme_OpenSource.txt
//*******************************************************************
//
// This Module contains code derived from
// - none
//*******************************************************************
// For ESP32-DevKitC PIN Configuration look at BeeIoT.h
//
//*******************************************************************
// SPI local Libraries
//*******************************************************************

#include <Arduino.h>
#include <stdio.h>
#include <esp_log.h>
#include "sdkconfig.h"
#include <SPI.h>

#include "beeiot.h" // provides all GPIO PIN configurations of all sensor Ports !


// Libraries for SD card at ESP32_
// ...has support for FAT32 support with long filenames
#include "FS.h"
#include "SD.h"
// #include "SPI.h"
#include "sdcard.h"
#include "LoRa.h"

// Libs for WaveShare ePaper 2.7 inch r/w/b Pinning GxGDEW027C44
#include <GxEPD.h>
#include <GxGDEW027C44/GxGDEW027C44.h>  // 2.7" b/w/r
#include <GxIO/GxIO_SPI/GxIO_SPI.h>
#include <GxIO/GxIO.h>

// #include "BitmapGraphics.h"
#include "BitmapExamples.h"
#include "BitmapWaveShare.h"


//************************************
// Global data object declarations
//**********************************
extern uint16_t	lflags;      // BeeIoT log flag field


// ePaper IO-SPI Object: arbitrary selection of DC + RST + BUSY
// for defaults: C:\Users\MCHRESSE\.platformio\packages\framework-arduinoespressif32\variants\esp32
GxIO_Class io(SPI, EPD_CS, EPD_DC, EPD_RST); // (SPIclass, EPD_CS, DC, RST, backlight=0) 
GxEPD_Class display(io, EPD_RST, EPD_BUSY);  // (io-class GxGDEW027C44, RST, BUSY)

// #define SPISPEED    2000000  // SPI speed: 2MHz
// static const int spiClk = SPISPEED; // 2 MHz

int isepd =-1;          // =0 ePaper found
int issdcard =-1;       // =0 SDCard found

//*******************************************************************
// SPI Port Setup Routine for 2 SPI ports: SDCard + ePaper  + LoRa Module at VSPI
//*******************************************************************
int setup_spi_VSPI() {    // My SPI Constructor
    BHLOG(LOGSPI) Serial.println("  MSPI: VSPI port for 3 devices");

// First disabe all SPI devices CS line to avoid collisions
    pinMode(EPD_CS, OUTPUT);    //VSPI SS for ePaper EPD
    pinMode(SD_CS,  OUTPUT);    //HSPI SS for SDCard Port
    pinMode(BEE_CS, OUTPUT);
    digitalWrite(EPD_CS, HIGH);
    digitalWrite(SD_CS, HIGH);
    digitalWrite(BEE_CS, HIGH);

// Setup VSPI BUS
    pinMode(VSPI_SCK, OUTPUT);
    pinMode(VSPI_MISO, INPUT_PULLUP);
    pinMode(VSPI_MOSI, OUTPUT);
    digitalWrite(VSPI_SCK, HIGH);
    digitalWrite(VSPI_MOSI, HIGH);

// Preset SPI dev: BEE-LoRa Module
    pinMode(BEE_RST, OUTPUT);
    pinMode(BEE_DIO0, INPUT_PULLUP);
    pinMode(BEE_DIO1, INPUT_PULLUP);
    pinMode(BEE_DIO2, INPUT_PULLUP);
    digitalWrite(BEE_RST, HIGH);
//    digitalWrite(BEE_MOSI, HIGH); // done via VSPI_MOSI if the same

// Preset SPI dev: BEE-LoRa Module
    pinMode(EPD_RST, OUTPUT);
    digitalWrite(EPD_RST, HIGH);

    BHLOG(LOGSPI) Serial.print("  MSPI: SPI-Init of SD card...");
    if (!SD.begin(SD_CS)){
        issdcard = -1;
        BHLOG(LOGSPI) Serial.println("  MSPI: SD Card Mount Failed");
    } else {
        BHLOG(LOGSPI) Serial.println("  MSPI: SD Card mounted");
        issdcard = 0;
    }

     // override the default CS, reset, and IRQ pins (optional)
    LoRa.setPins(BEE_CS, BEE_RST, BEE_DIO0);// set CS, reset, IRQ pin

    #ifdef EPD_CONFIG
    BHLOG(LOGSPI) Serial.println("  MSPI: SPI-Init: ePaper EPD part1 ...");
    display.init();   // enable diagnostic output on Serial
    // display class does not provide any feedback if dev. is available
    // so we assume its there...
    isepd = 0;    
    #endif

    return (isepd + issdcard);   // SPI port is initialized
} // end of setup_spi_sd_epd()
