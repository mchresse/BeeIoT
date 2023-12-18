// Copyright 2023 Randolph Esser. All rights reserved.
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

//#ifndef EPAPER_H
//#define EPAPER_H

#ifdef EPD2_CONFIG
// for GxEPD2 library support only

// Define ePaper Size-type
//#define EPD27_CONFIG	0	// EPD2 ePaper with 2.7"
#define EPD29_CONFIG	0	// EPD2 ePaper with 2.9"


// EPD Colour Mode may work for B/W and B/W/R displays if red is not used -> faster
#define EPD_BW				// Define EPD Type: Black/White only

#define ENABLE_GxEPD2_GFX 0
//#include <GFX.h>
#include <GxEPD2_GFX.h>				// from ZinggJM/GxEPD (https://github.com/ZinggJM/GxEPD)
#include <GxEPD2_BW.h>
#include <GxEPD2_3C.h>

// select the display class (only one), matching the kind of display panel
#define GxEPD2_DISPLAY_CLASS GxEPD2_BW
//#define GxEPD2_DISPLAY_CLASS GxEPD2_3C
//#define GxEPD2_DISPLAY_CLASS GxEPD2_7C
#ifdef EPD27_CONFIG
#define GxEPD2_DRIVER_CLASS GxEPD2_270     // GDEW027W3   176x264, EK79652 (IL91874), (WFI0190CZ22)
#endif
//#define GxEPD2_DRIVER_CLASS GxEPD2_270_GDEY027T91 // GDEY027T91 176x264, SSD1680, (FB)
#ifdef EPD29_CONFIG
#define GxEPD2_DRIVER_CLASS GxEPD2_290     // GDEH029A1   128x296, SSD1608 (IL3820), (E029A01-FPC-A1 SYX1553)
#endif
//#define GxEPD2_DRIVER_CLASS GxEPD2_290_T5  // GDEW029T5   128x296, UC8151 (IL0373), (WFT0290CZ10)
//#define GxEPD2_DRIVER_CLASS GxEPD2_290_T5D // GDEW029T5D  128x296, UC8151D, (WFT0290CZ10)
//#define GxEPD2_DRIVER_CLASS GxEPD2_290_I6FD // GDEW029I6FD  128x296, UC8151D, (WFT0290CZ10)
//#define GxEPD2_DRIVER_CLASS GxEPD2_290_T94 // GDEM029T94  128x296, SSD1680, (FPC-7519 rev.b)
//#define GxEPD2_DRIVER_CLASS GxEPD2_290_T94_V2 // GDEM029T94  128x296, SSD1680, (FPC-7519 rev.b), Waveshare 2.9" V2 variant
//#define GxEPD2_DRIVER_CLASS GxEPD2_290_BS // DEPG0290BS  128x296, SSD1680, (FPC-7519 rev.b)
//#define GxEPD2_DRIVER_CLASS GxEPD2_290_M06 // GDEW029M06  128x296, UC8151D, (WFT0290CZ10)
//#define GxEPD2_DRIVER_CLASS GxEPD2_290_GDEY029T94 // GDEY029T94 128x296, SSD1680, (FPC-A005 20.06.15)
// 3-color e-papers
//#define GxEPD2_DRIVER_CLASS GxEPD2_270c     // GDEW027C44  176x264, IL91874, (WFI0190CZ22)
//#define GxEPD2_DRIVER_CLASS GxEPD2_290c     // GDEW029Z10  128x296, UC8151 (IL0373), (WFT0290CZ10)
//#define GxEPD2_DRIVER_CLASS GxEPD2_290_Z13c // GDEH029Z13  128x296, UC8151D, (HINK-E029A10-A3 20160809)
//#define GxEPD2_DRIVER_CLASS GxEPD2_290_C90c // GDEM029C90  128x296, SSD1680, (FPC-7519 rev.b)

// somehow there should be an easier way to do this
#define GxEPD2_BW_IS_GxEPD2_BW true
#define GxEPD2_3C_IS_GxEPD2_3C true
#define GxEPD2_7C_IS_GxEPD2_7C true
#define GxEPD2_1248_IS_GxEPD2_1248 true
#define IS_GxEPD(c, x) (c##x)
#define IS_GxEPD2_BW(x) IS_GxEPD(GxEPD2_BW_IS_, x)
#define IS_GxEPD2_3C(x) IS_GxEPD(GxEPD2_3C_IS_, x)
#define IS_GxEPD2_7C(x) IS_GxEPD(GxEPD2_7C_IS_, x)
#define IS_GxEPD2_1248(x) IS_GxEPD(GxEPD2_1248_IS_, x)

#if defined(ESP32)
#define MAX_DISPLAY_BUFFER_SIZE 65536ul // e.g.
#if IS_GxEPD2_BW(GxEPD2_DISPLAY_CLASS)
#define MAX_HEIGHT(EPD) (EPD::HEIGHT <= MAX_DISPLAY_BUFFER_SIZE / (EPD::WIDTH / 8) ? EPD::HEIGHT : MAX_DISPLAY_BUFFER_SIZE / (EPD::WIDTH / 8))
#elif IS_GxEPD2_3C(GxEPD2_DISPLAY_CLASS)
#define MAX_HEIGHT(EPD) (EPD::HEIGHT <= (MAX_DISPLAY_BUFFER_SIZE / 2) / (EPD::WIDTH / 8) ? EPD::HEIGHT : (MAX_DISPLAY_BUFFER_SIZE / 2) / (EPD::WIDTH / 8))
#elif IS_GxEPD2_7C(GxEPD2_DISPLAY_CLASS)
#define MAX_HEIGHT(EPD) (EPD::HEIGHT <= (MAX_DISPLAY_BUFFER_SIZE) / (EPD::WIDTH / 2) ? EPD::HEIGHT : (MAX_DISPLAY_BUFFER_SIZE) / (EPD::WIDTH / 2))
#endif
#endif //ESP32

#define EPD_SerDiagRate	0			// EPD Serial bitrate in debug mode:115200
#define EPD_RESET_PULSE	2			// length of Reset pulse in [ms]

// BIoT Data Frame Format:
#define newline12	19
#define newline9	16
#define shiftx		4
#define dataupd_x	shiftx + (13 * 10)		// 134

#define theader_x	shiftx
#define theader_y	newline9
#define ttime_x		shiftx
#define ttime_y		theader_y + newline9 +1	// 33
#define tbox_x		shiftx
#define tbox_y		ttime_y + 2 + 2			// 36
#define tweight_x	tbox_x + shiftx
#define tweight_y	tbox_y + newline9		// 52
#define ttemphive_x	tbox_x + shiftx
#define ttemphive_y	tweight_y + newline9 	// 68
#define ttempext_x	tbox_x + shiftx
#define ttempext_y	ttemphive_y + newline9	// 84
#define tbatt_x		tbox_x + shiftx
#define tbatt_y		ttempext_y + newline9	// 100
#define tbox_wx		296 - shiftx
#define tbox_hy		72 						// tbatt_y - tbox_y + newline9 - 8
#define tstatus_x	tbox_x + shiftx
#define tstatus_y	tbox_y + tbox_hy + newline9 -1 // 123

// EPD2_GFX Library prototypes
void epd2_test(void);
void helloWorld(void);
void helloWorldForDummies(void);
void helloFullScreenPartialMode(void);
void helloArduino(void);
void helloEpaper(void);
void helloValue(double v, int digits);
void showFont(const char name[], const GFXfont* f);
void drawFont(const char name[], const GFXfont* f);
void drawBitmaps(void);
void drawBitmaps128x296(void);
void drawBitmaps176x264(void);
void showPartialUpdate(void);
void deepSleepTest(void);
void drawGraphics(void);

int  setup_epd2		(int mode);		// in epd2.cpp
void showdata		(void);
void showdata27		(void);
void showdata29_base(void);
void showdata29_update(void);

void showbeacon2 	(void);

#endif // EPD2_CONFIG
//#endif // EPAPER_H