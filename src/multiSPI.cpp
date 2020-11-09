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
#include <stdint.h>

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
// #include <GxGDEW027C44/GxGDEW027C44.h> // 2.7" b/w/r
#include <GxGDEW027W3/GxGDEW027W3.h>     // 2.7" b/w
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
// With defined WROVERB switch: EPD_BUSY = 39 !!!
GxIO_Class io(SPI, EPD_CS, EPD_DC, EPD_RST, EPD_BUSY); // (SPIclass, EPD_CS, DC, RST, backlight=0)
GxEPD_Class display(io, EPD_RST, EPD_BUSY);  // (io-class GxGDEW027C44, RST, BUSY)

// #define SPISPEED    2000000  // SPI speed: 2MHz
// static const int spiClk = SPISPEED; // 2 MHz

int isepd =0;          // =1 ePaper found
int issdcard =0;       // =1 SDCard found

//*******************************************************************
// SPI Port Setup Routine for 2 SPI ports: SDCard + ePaper  + LoRa Module at VSPI
//*******************************************************************
int setup_spi_VSPI(int reentry) {    // My SPI Constructor
    BHLOG(LOGSPI) Serial.println("  MSPI: VSPI port for 3 devices");
    isepd = 0;
    issdcard = 0;

// First disabe all SPI devices CS line to avoid collisions
    pinMode(EPD_CS, OUTPUT);    //VSPI SS for ePaper EPD
    pinMode(SD_CS,  OUTPUT);    //HSPI SS for SDCard Port
    pinMode(BEE_CS, OUTPUT);
    digitalWrite(EPD_CS, HIGH);
    digitalWrite(SD_CS, HIGH);
    digitalWrite(BEE_CS, HIGH);
    gpio_hold_dis(GPIO_NUM_2);   // enable SD_CS
    gpio_hold_dis(GPIO_NUM_5);   // enable EPD_CS
    gpio_hold_dis(GPIO_NUM_12);  // enable BEE_CS

// Setup VSPI BUS
    pinMode(VSPI_MISO, INPUT_PULLUP);
    pinMode(VSPI_MOSI, OUTPUT);
    pinMode(VSPI_SCK, OUTPUT);
    digitalWrite(VSPI_SCK, HIGH);
    digitalWrite(VSPI_MOSI, HIGH);

// Preset SPI dev: BEE-LoRa Module
    pinMode(BEE_RST, OUTPUT);
    digitalWrite(BEE_RST, HIGH);
    gpio_hold_dis(GPIO_NUM_14);  // enable BEE_RST
    pinMode(BEE_DIO0, INPUT);
    pinMode(BEE_DIO1, INPUT);
    pinMode(BEE_DIO2, INPUT);
//    digitalWrite(BEE_MOSI, HIGH); // done via VSPI_MOSI if the same
    LoRa.setPins(BEE_CS, BEE_RST, BEE_DIO0);// set CS, reset, IRQ pin

// Preset SPI dev: EPD Display Module
    pinMode(EPD_RST, OUTPUT);
    digitalWrite(EPD_RST, HIGH);
    pinMode(EPD_DC, OUTPUT);
    digitalWrite(EPD_DC, HIGH);
    pinMode(EPD_BUSY, INPUT);

    #ifdef SD_CONFIG
        if (!SD.begin(SD_CS)){
            BHLOG(LOGSPI) Serial.println("  MSPI: SD Card Mount Failed");
        } else {
            BHLOG(LOGSPI) Serial.println("  MSPI: SD Card mounted");
            issdcard = 1;
        }
    #endif

    #ifdef EPD_CONFIG
		BHLOG(LOGSPI) Serial.println("  MSPI: SPI-Init: ePaper EPD part1 ...");
		display.init();   // enable diagnostic output on Serial
		// display class does not provide any feedback if dev. is available
		// so we assume its there...
		isepd = 1;
    #endif

    return (issdcard);   // SD SPI port is initialized
} // end of setup_spi_sd_epd()
