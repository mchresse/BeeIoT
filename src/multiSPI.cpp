
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
#include "BeeIoTWan.h"

// Libraries for SD card at ESP32_
// ...has support for FAT32 support with long filenames
#include "SPI.h"
#include "FS.h"
#include "SD.h"
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
extern dataset	bhdb;		// central node status DB -> main.cpp
extern uint16_t	lflags;     // BeeIoT log flag field

// instantiate default SPI device: HSPI
// SPI Pin defintion see beeiot.h
#define SPISPEED    1000000  // SPI speed: 1MHz
SPIClass SPI2(HSPI); // create SPI2 object with default HSPI pinning

// ePaper IO-SPI Object: arbitrary selection of DC + RST + BUSY
// for defaults: C:\Users\MCHRESSE\.platformio\packages\framework-arduinoespressif32\variants\esp32
// With defined WROVERB switch: EPD_BUSY = 39 !!!
GxIO_Class io(SPI2, EPD_CS, EPD_DC, EPD_RST, 0); // (SPIclass, EPD_CS, DC, RST, backlight=0)
GxEPD_Class display(io, EPD_RST, EPD_BUSY);  // (io-class GxGDEW027C44, RST, BUSY)

// static const int spiClk = SPISPEED; // 2 MHz

int isepd =0;          // =1 ePaper found
int issdcard =0;       // =1 SDCard found

//*******************************************************************
// SPI Port Setup Routine for 2 SPI ports: SDCard + ePaper  + LoRa Module at VSPI
//*******************************************************************
int setup_spi(int reentry) {    // My SPI Constructor
    BHLOG(LOGSPI) Serial.println("  MSPI: VSPI port for 3 devices");
    isepd = 0;
    issdcard = 0;

// First disabe all SPI devices CS line to avoid collisions
    pinMode(EPD_CS, OUTPUT);    	//VSPI SS for ePaper EPD
    pinMode(SD_CS,  OUTPUT);    	//HSPI SS for SDCard Port
    pinMode(LoRa_CS, OUTPUT);
    digitalWrite(EPD_CS, HIGH);
    digitalWrite(SD_CS, HIGH);
    digitalWrite(LoRa_CS, HIGH);
    gpio_hold_dis(SD_CS);   		// enable SD_CS
    gpio_hold_dis(EPD_CS);   		// enable EPD_CS
    gpio_hold_dis(LoRa_CS);  		// enable BEE_CS

// Activate EPD low sid switch -> connect ground to epaper
	pinMode(EPD_LOWSW, OUTPUT);
	digitalWrite(EPD_LOWSW, LOW);
    gpio_hold_dis(EPD_LOWSW);  		// enable BEE_CS

// Setup SPI BUS
    pinMode(SPI_MISO, INPUT);
    pinMode(SPI_MOSI, OUTPUT);
    pinMode(SPI_SCK, OUTPUT);
    digitalWrite(SPI_SCK, HIGH);
    digitalWrite(SPI_MOSI, HIGH);
	gpio_hold_dis(SPI_MISO);
	gpio_hold_dis(SPI_MOSI);
	gpio_hold_dis(SPI_SCK);

	SPI2.begin(SPI_SCK, SPI_MISO, SPI_MOSI, LoRa_CS);

// Preset SPI dev: LoRa Module
    pinMode(LoRa_RST, OUTPUT);
    digitalWrite(LoRa_RST, HIGH);
    gpio_hold_dis(LoRa_RST);
    pinMode(LoRa_DIO0, INPUT);
    gpio_hold_dis(LoRa_DIO0);
//    pinMode(LoRa_DIO1, INPUT);		// n.a.
//    pinMode(LoRa_DIO2, INPUT);		// n.a.

// Configure LoRa Port
	LoRa.setSPI(SPI2);
    LoRa.setPins(LoRa_CS, LoRa_RST, LoRa_DIO0); // set CS, reset, IRQ pin

// Preset SPI dev: EPD Display Module
    pinMode(EPD_BUSY, INPUT);
    pinMode(EPD_DC, OUTPUT);
    pinMode(EPD_RST, OUTPUT);
    digitalWrite(EPD_DC, HIGH);
    digitalWrite(EPD_RST, HIGH);
    gpio_hold_dis(EPD_DC);
    gpio_hold_dis(EPD_RST);
    gpio_hold_dis(EPD_BUSY);

    #ifdef SD_CONFIG
		if(bhdb.hwconfig & HC_SDCARD){
			if (!SD.begin(SD_CS, SPI2)){
				BHLOG(LOGSPI) Serial.println("  MSPI: SD Card Mount Failed");
			} else {
				BHLOG(LOGSPI) Serial.println("  MSPI: SD Card mounted");
				issdcard = 1;
			}
		}else{	// SDCard disabled by GW side HWconf flag
			issdcard=0;
		}
    #endif

    #ifdef EPD_CONFIG
		BHLOG(LOGSPI) Serial.println("  MSPI: SPI-Init: ePaper EPD part1 ...");
		display.init(0);   // enable diagnostic output on Serial if serial_diag_bitrate is set >0
		// display class does not provide any feedback if device is available
		// so we have to assume its there...
		isepd = 1; // have to assume epd works now...no check implememnted by driver
    #endif

    return (issdcard);   // SD SPI port is initialized
} // end of setup_spi_sd_epd()
