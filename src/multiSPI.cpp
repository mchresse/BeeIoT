
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

#ifdef EPD_CONFIG
// Libs for WaveShare ePaper 2.7 inch r/w/b Pinning GxGDEW027C44
#include <GxEPD.h>
// #include <GxGDEW027C44/GxGDEW027C44.h>	// 2.7" b/w/r
#include <GxGDEW027W3/GxGDEW027W3.h>     // 2.7" b/w
//#include <GxGDEM029T94/GxGDEM029T94.h>		// 2.9" b/w

#include <GxIO/GxIO_SPI/GxIO_SPI.h>
#include <GxIO/GxIO.h>

// #include "BitmapGraphics.h"
#include "BitmapExamples.h"
#include "BitmapWaveShare.h"
#endif // EPD_CONFIG

#include "epaper.h"
#include <Fonts/FreeMonoBold9pt7b.h>



//************************************
// Global data object declarations
//**********************************
extern dataset	bhdb;		// central node status DB -> main.cpp
extern uint16_t	lflags;     // BeeIoT log flag field

// instantiate default SPI device: HSPI
// SPI Pin defintion see beeiot.h
#define SPISPEED    1000000  	// SPI speed: 1MHz
SPIClass SPI2(HSPI); 			// create SPI2 object with default HSPI pinning

#ifdef EPD_CONFIG
// ePaper IO-SPI Object: arbitrary selection of DC + RST + BUSY
// for defaults: C:\Users\MCHRESSE\.platformio\packages\framework-arduinoespressif32\variants\esp32
// With defined WROVERB switch: EPD_BUSY = 39 !!!
GxIO_Class io(SPI2, EPD_CS, EPD_DC, EPD_RST, 0); // (SPIclass, EPD_CS, DC, RST, backlight=0)
GxEPD_Class display(io, EPD_RST, EPD_BUSY);  // (io-class GxGDEW027C44, RST, BUSY)
#else
// GxEPD2 support
#if defined(ESP32)
// Now define the one and only Display <template> class instance of a EPD2 device
GxEPD2_DISPLAY_CLASS<GxEPD2_DRIVER_CLASS, MAX_HEIGHT(GxEPD2_DRIVER_CLASS)>
	display(GxEPD2_DRIVER_CLASS(/*CS=*/ EPD_CS, /*DC=*/ EPD_DC, /*RST=*/ EPD_RST, /*BUSY=*/ EPD_BUSY));
#endif // ESP32
#endif // EPD_CONFIG

// static const int spiClk = SPISPEED; // 2 MHz

int isepd =0;          // =1 ePaper found
int issdcard =0;       // =1 SDCard found

void helloWorldx(void);

//*******************************************************************
// SPI Port Setup Routine for 2 SPI ports: SDCard + ePaper  + LoRa Module at VSPI
//*******************************************************************
int setup_spi(int reentry) {    // My SPI Constructor
    BHLOG(LOGSPI) Serial.println("  MultiSPI: Setup HSPI port for 3 devices");
    isepd = 0;
    issdcard = 0;

// all related GPIO pins have been initialized by gpio_init() already

// Configure Master SPI Port
	digitalWrite(SPIPWREN, HIGH);	// enable SPI Power (for all SPI devices)
	digitalWrite(EPDGNDEN, LOW);   // enable EPD Ground low side switch
	delay(10);						// give PwrUp some time

	//Instantiate SPI port by HSPI default pins
	SPI2.begin(SPI_SCK, SPI_MISO, SPI_MOSI, LoRa_CS);

// Configure attached LoRa Port
	LoRa.setSPI(SPI2);
    LoRa.setPins(LoRa_CS, LoRa_RST, LoRa_DIO0); // set CS, Reset, IRQ pin

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
		digitalWrite(EPDGNDEN, LOW);   // enable EPD Ground low side switch

		display.init(0);   // enable diagnostic output on Serial if serial_diag_bitrate is set >0
		// display class does not provide any feedback if device is available
		// so we have to assume its there...
		isepd = 1; // have to assume epd works now...no check implememnted by driver
    #else
		BHLOG(LOGSPI) Serial.println("  MSPI: SPI-Init: ePaper EPD2 part1 ...");
		display.epd2.selectSPI(SPI2, SPISettings(SPISPEED, MSBFIRST, SPI_MODE0));
		display.init(115200);   // enable diagnostic output on Serial if serial_diag_bitrate is set >0

		// display class does not provide any feedback if device is available
		// so we have to assume its there...
		isepd = 1; // have to assume epd works now...no check implememnted by driver
	#endif

    return (issdcard);   // SD SPI port is initialized
} // end of setup_spi_sd_epd()
