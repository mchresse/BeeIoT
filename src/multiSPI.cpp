
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

#include "epaper.h"



//************************************
// Global data object declarations
//**********************************
extern dataset	bhdb;		// central node status DB -> main.cpp
extern uint16_t	lflags;     // BeeIoT log flag field

// instantiate default SPI device: HSPI
// SPI Pin defintion see beeiot.h
#define SPISPEED    4000000  	// SPI speed: 4MHz
SPIClass SPI2(HSPI); 			// create SPI2 object with default HSPI pinning

#ifdef EPD_CONFIG
	// ePaper IO-SPI Object: arbitrary selection of DC + RST + BUSY
	// for defaults: C:\Users\MCHRESSE\.platformio\packages\framework-arduinoespressif32\variants\esp32
	// With defined WROVERB switch: EPD_BUSY = 39 !!!
	GxIO_Class io(SPI2, EPD_CS, EPD_DC, EPD_RST, 0); // (SPIclass, EPD_CS, DC, RST, backlight=0)
	GxEPD_Class display(io, EPD_RST, EPD_BUSY);  // (io-class GxGDEW027C44, RST, BUSY)
#else
	#ifdef EPD2_CONFIG
		// GxEPD2 support
		#if defined(ESP32)
		// Now define the one and only Display <template> class instance of a EPD2 device
RTC_DATA_ATTR GxEPD2_DISPLAY_CLASS<GxEPD2_DRIVER_CLASS, MAX_HEIGHT(GxEPD2_DRIVER_CLASS)>
			display(GxEPD2_DRIVER_CLASS(/*CS=*/ EPD_CS, /*DC=*/ EPD_DC, /*RST=*/ EPD_RST, /*BUSY=*/ EPD_BUSY));
		#endif // ESP32
	#endif // EPD2_CONFIG
#endif // EPD_CONFIG


int isepd =0;          // =1 ePaper found
int issdcard =0;       // =1 SDCard found
extern bool	EPDupdate; // EPD update control requested
extern bool GetData;   // =1 -> manual trigger by blue key to do next sensor loop


int SetonKey(int key, void(*callback)(void));
void onKey1(void);		// ISR for EPD_KEY1 -> Yellow Button
void onKey2(void);		// ISR for EPD_KEY2 -> Red	  Button
void onKey3(void);		// ISR for EPD_KEY3 -> Green  Button
void onKey4(void);		// ISR for EPD_KEY4 -> Blue   Button


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
	digitalWrite(EPDGNDEN, LOW);    // enable EPD Ground low side switch
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
		#ifdef EPD2_CONFIG
			BHLOG(LOGSPI) Serial.println("  MSPI: SPI-Init: ePaper EPD2 part1 ...");
			display.epd2.selectSPI(SPI2, SPISettings(SPISPEED, MSBFIRST, SPI_MODE0));
			if(!reentry){
				display.init(0);   // enable diagnostic output on Serial if serial_diag_bitrate is set >0
			}else{
				digitalWrite(EPDGNDEN, LOW);   		// enable EPD Pwr-Ground by low side switch
				display.init(0, false, 2, false);   // init in update mode -> no powerloss during deepsleep allowed
			}
			// display class does not provide any feedback if device is available
			// so we have to assume its there...
			isepd = 1; // have to assume epd works now...no check implememnted by driver
		#else
			isepd = 0; // have to assume epd not existing
		#endif
	#endif

// Preset Keys 1-4: active 0 => connects to GND (pullup required)
// KEY1 => (Deep-) Sleep Wakeup trigger
// KEY2 => test IRQ		 e.g. usefull for MCU_Reset (on EN)
// KEY3 => n.a.
// KEY4 => n.a.

// Assign Key-IRQ to callback function
	SetonKey(1, onKey1);
	SetonKey(2, onKey2);
//	SetonKey(3, onKey3);	// not supported for BIoT v4.x PCB
//	SetonKey(4, onKey4);	// not supported for BIoT v4.x PCB

    return (issdcard);   // SD SPI port is initialized
} // end of setup_spi_sd_epd()

//*************************************************************************
// SetonKey()
// Assign ISR to EPD-Keyx	(1: Yellow, 2: Red, 3: Green, 4: Blue)
// PIN-Keys already defined in beeiot.h
// Input:
//   key		1..4	Key number of EPD board (PIN definition see also beeiot.h)
//   callback	*ptr	Function ptr to be assigned to IRQ of Key-GPIO
// Return:
//	0			Callback assignd to IRQ of KEYx Port
// -1			Wrong/reserved  key #
// -2			GPIO is not ready for ISR assignment
//*************************************************************************
int SetonKey(int key, void(*callback)(void)){
int rc=0;

	if(!callback)	// NULL ptr. can not be assigned
 		return(-1);

	switch (key){
	case 1: 			// EPD_KEY 1:
		if(callback){
			// Key1 setup as wakeup trigger in prepare_sleep_mode() already
//			pinMode(EPD_KEY1, INPUT);
//    	  	attachInterrupt(digitalPinToInterrupt(EPD_KEY1), callback, RISING);
			BHLOG(LOGEPD) Serial.println("  SetonKey: EPD-Key1 assigned as Wakeup call");
			rc= 0;
		}else{
			BHLOG(LOGEPD) Serial.println("  SetonKey: EPD-Key1 without callback");
	    	detachInterrupt(digitalPinToInterrupt(EPD_KEY1));
			rc= -2;
		}
		break;
	case 2: 			// EPD_KEY 2
		if(callback){
			pinMode(EPD_KEY2, INPUT);
	  		attachInterrupt(digitalPinToInterrupt(EPD_KEY2), callback, RISING);
			BHLOG(LOGEPD) Serial.printf("  SetonKey: EPD-Key2 qassigned to GPIO%i-IRQ", EPD_KEY2);
			rc= 0;
		}else{
			BHLOG(LOGEPD) Serial.println("  SetonKey: EPD-Key2 without callback");
	    	detachInterrupt(digitalPinToInterrupt(EPD_KEY2));
			rc= -2;
		}
		break;
	case 3: 			// EPD_KEY 3 /w ext 10k Pullup to 3.3V!
		if(callback){
//			pinMode(EPD_KEY3, INPUT);
//	    	attachInterrupt(digitalPinToInterrupt(EPD_KEY3), callback, RISING);
			BHLOG(LOGEPD) Serial.println("  SetonKey: EPD-Key3 undefined for V4.x board");
			rc= 0;
		}else{
//	    	detachInterrupt(digitalPinToInterrupt(EPD_KEY3));
			BHLOG(LOGEPD) Serial.println("  SetonKey: EPD-Key3 without callback");
			rc= -2;
		}
		break;
	case 4: 			// EPD_KEY 4 /w ext 10k Pullup to 3.3V!
		if(callback){
//			pinMode(EPD_KEY4, INPUT);
//	   		attachInterrupt(digitalPinToInterrupt(EPD_KEY4), callback, RISING);
			BHLOG(LOGEPD) Serial.println("  SetonKey: EPD-Key4 undefined for V4.x board");
			rc= 0;
		}else{
//	    	detachInterrupt(digitalPinToInterrupt(EPD_KEY4));
			BHLOG(LOGEPD) Serial.println("  SetonKey: EPD-Key4 without callback");
			rc= -2;
		}
		break;
	default:
		rc =-1;		// unsupported Key number
		BHLOG(LOGEPD) Serial.printf("  SetonKey: EPD-Key%i undefined for V4.x board", key);
		break;
	}

	return(rc);
}

//*************************************************************************
// onKeyx() ISR of EPD-Keyx	-> Yellow Button
void IRAM_ATTR onKey1(void){
	BHLOG(LOGBH) Serial.println("  onKey1: EPD-Key1 IRQ: Green Key");
	GetData =1;		// manual trigger to start next sensor collection loop (for MyDelay())
	EPDupdate=true;	// EPD update requested
}
//*************************************************************************
// onKeyx() ISR of EPD-Keyx	-> Red Button
void IRAM_ATTR onKey2(void){
	BHLOG(LOGBH) Serial.println("  onKey2: EPD-Key2 IRQ: Red Key");
	EPDupdate=true;	// EPD update requested
}
//*************************************************************************
// onKeyx() ISR of EPD-Keyx	-> Green Button
void IRAM_ATTR onKey3(void){
	BHLOG(LOGBH) Serial.println("  onKey3: EPD-Key3 IRQ: Green Key");
	EPDupdate=false;	// EPD update requested
}
//*************************************************************************
// onKeyx() ISR of EPD-Keyx	-> Blue button
void IRAM_ATTR onKey4(void){
	BHLOG(LOGBH) Serial.println("  onKey4: EPD-Key4 IRQ: Blue Key");
	EPDupdate=false;	// EPD update requested
}
