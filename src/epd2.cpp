//*******************************************************************
// epd2.cpp
// from Project https://github.com/mchresse/BeeIoT
//
// Description:
// ePaper Library functions based on GxEPD2 library
//
//----------------------------------------------------------
// Copyright (c) 2023-present, Randolph Esser
// All rights reserved.
// This file is distributed under the BSD-3-Clause License
// The complete license agreement can be obtained at:
//     https://github.com/mchresse/BeeIoT/license
// For used 3rd party open source see also Readme_OpenSource.txt
//*******************************************************************
//
// This Module contains code derived from
// - GxEPD2 library example code
//*******************************************************************

#include <Arduino.h>
#include <version.h>
#include <beeiot.h>

#ifdef EPD2_CONFIG

#include <BeeIoTWan.h>
#include <beelora.h>
#include <SPI.h>
#include <epaper.h>

// FreeFonts from Adafruit_GFX
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/FreeMonoBold18pt7b.h>
#include <Fonts/FreeMonoBold24pt7b.h>
#include <Fonts/FreeSansBold24pt7b.h>


//************************************
// Global data object declarations
//************************************
// extern GxEPD2_GFX& display;
extern GxEPD2_DISPLAY_CLASS<GxEPD2_DRIVER_CLASS, MAX_HEIGHT(GxEPD2_DRIVER_CLASS)>  display;

extern int isepd;			// =1 ePaper found
extern dataset bhdb;		// central BeeIo DB
extern uint16_t	lflags;		// BeeIoT log flag field
extern const char * beeiot_StatusString[];
extern byte BeeIoTStatus;
extern int ReEntry;			// =0 initial startup needed(after reset);   =1 after deep sleep;
							// =2 after light sleep; =3 ModemSleep Mode; =4 Active Wait Loop
extern bool	EPDupdate;		// =true: EPD update requested

#define Max_starts_between_refresh 5	// amount of data update befre refresh of full page
RTC_DATA_ATTR struct {
  uint8_t startup_count=0;
  bool clean_start=true;
} startup_data;


void biot_welcome_page(void);
void biot_welcome_full(void);
void pagetest(void);
void testx(void);

//*******************************************************************
// ePaper Setup Routine
//*******************************************************************
int setup_epd2(int reentry) {   // My EPD Constructor
// isepd is defined at multi SPI setup;

#ifdef EPD2_CONFIG
	// enter EPD active mode:
	digitalWrite(EPDGNDEN, LOW);   // enable EPD Ground low side switch

	if(!reentry){
		BHLOG(LOGEPD) Serial.println("    EPD2: Start ePaper Welcome");
		if(isepd){ // set some presets for a std. text field
			biot_welcome_page();
			startup_data.clean_start = true;
			startup_data.startup_count = Max_starts_between_refresh;
		}
	}
#endif
	return isepd;
}

//*********************************************************
// test function of EPD2 based 2.9" display from Waveshare
//*********************************************************
//*********************************************************
// biot_welcome()	pagewise
// Print BIot Welcome message (once after first POn or Reset
// based on GxEPD2 library with full screen update
//
// Global: display object of epaper SPI device
//*********************************************************
void biot_welcome_page(){
const char theader[] ="    BeeIoT";
const char tauthor[] ="        by R.Esser";

	display.setRotation(1);    // 1 + 3: print in horizontal format
	display.setTextColor(GxEPD_BLACK);
	display.setFullWindow();

	display.firstPage();
	do
	{
		display.fillScreen(GxEPD_WHITE); 	// set the background to white (fill the buffer with value for white)
		display.setFont(&FreeMonoBold18pt7b);
		display.setCursor(0, 2*12);
		display.println(theader);

		display.setFont(&FreeMonoBold12pt7b);
		display.printf ("      v%s.%s.%s\n", VMAJOR, VMINOR, VBUILD);

		display.setFont(&FreeMonoBold9pt7b);
		display.println(tauthor);
		display.setFont(&FreeMonoBold9pt7b);
		display.printf ("      BoardID: %08X\n", (uint32_t)bhdb.BoardID);
	}
	// tell the graphics class to transfer the buffer content (page) to the controller buffer
	// the graphics class will command the controller to refresh to the screen when the last page has been transferred
	// returns true if more pages need be drawn and transferred
	// returns false if the last page has been transferred and the screen refreshed for panels without fast partial update
	// returns false for panels with fast partial update when the controller buffer has been written once more, to make the differential buffers equal
	// (for full buffered with fast partial update the (full) buffer is just transferred again, and false returned)
	while (display.nextPage());
}



//*********************************************************
// showdata2()
// Show Sensor log data on epaper Display
	// Input: sampleID= Index ond Sensor dataset from BHDB
//*********************************************************

void showdata(void){
	if(bhdb.hwconfig & HC_EPD) {	// EPD access enabled by PCFG
		// No EPD panel connected or no Update request pending
		if(isepd==0 || 	EPDupdate==false){
			return;	// no EPD port -> no action
		}
		digitalWrite(EPDGNDEN, LOW);   // enable EPD Ground low side switch

#ifdef EPD27_CONFIG
		showdata27();			// 2.7" frame Base + Data update in full window refresh mode
#endif
#ifdef EPD29_CONFIG
		if (startup_data.startup_count >= Max_starts_between_refresh)
		{	// full window refresh required frequently
			startup_data.startup_count = 0;
			startup_data.clean_start = true;
			showdata29_base();			// Base update in full Window refresh mode
		}else{
			showdata29_updbase();		// Base update in partial upd. mode on full screen
		}

		showdata29_update();			// Do data update only in partial window mode
		display.powerOff();

		startup_data.clean_start = false;
		startup_data.startup_count++;	// count time till fullr efresh
#endif
		// enter EPD low power mode
		digitalWrite(EPDGNDEN, LOW); 	// keep EPD Ground low side switch enabled
		EPDupdate=false;				// EPD update request completed -> reset flag
	}else{
		digitalWrite(EPDGNDEN, HIGH); 	// No EPD: Disable EPD Ground low side switch for power saving
	}
}



void showdata27(void){
#ifdef EPD27_CONFIG
  	display.setRotation(1);    // 1 + 3: print in horizontal format
	display.fillScreen(GxEPD_WHITE);

 	 	display.setTextColor(GxEPD_BLACK);
  	display.setFont(&FreeMonoBold9pt7b);
  	display.setCursor(0, 0);
  	display.println();          // adjust cursor to lower left corner of char row

	display.setFont(&FreeMonoBold12pt7b);
  	display.printf("BeeIoTv%s.%s #%i C%i", VMAJOR, VMINOR, bhdb.loopid, bhdb.chcfgid);
  	display.setFont(&FreeMonoBold9pt7b);
  	display.println();
  	display.printf("  %s %s", bhdb.date, bhdb.time);

  	display.setFont(&FreeMonoBold12pt7b);
  	display.println();

	display.drawRect(4, 12+9+9+12, display.width()-(4+4), 94, GxEPD_BLACK);

	display.printf(" Gewicht: %skg\n", 	String(bhdb.dlog.HiveWeight,3));
	display.printf(" Temp.Beute: %s\n", String(bhdb.dlog.TempHive,1));
	display.printf(" TempExtern: %s\n", String(bhdb.dlog.TempExtern,1));
	display.printf(" TempIntern: %s", 	String(bhdb.dlog.TempIntern,1));

	display.setFont(&FreeMonoBold9pt7b);	// -> 24chars/line
  	display.println();

	display.printf(" Batt[V]:%s(%s%%)<%s\n",
			String((float)bhdb.dlog.BattLoad/1000,2),
			String((uint16_t)bhdb.dlog.BattLevel),
			String((float)bhdb.dlog.BattCharge/1000,2));

	display.setFont(&FreeMonoBold9pt7b);	// -> 24chars/line
    display.print(" Status: ");
	if(BeeIoTStatus == BIOT_SLEEP && ReEntry == 1){
  		display.print(beeiot_StatusString[BIOT_DEEPSLEEP]);
	}else{
  		display.print(beeiot_StatusString[BeeIoTStatus]);
	}

	display.display();				// WakeUp -> update Display
#endif
} // end of ShowData27()


//*********************************************************
// showdata29()
// Show Sensor log data on epaper Display
// Input: sampleID= Index ond Sensor dataset from BHDB
//*********************************************************
void showdata29(void){
	display.setRotation(1);					// 1 + 3: print in horizontal format
	display.fillScreen(GxEPD_WHITE);
	display.setTextColor(GxEPD_BLACK);
	display.setFont(&FreeMonoBold12pt7b);   // -> 22chars/line (13 dots/char)
	display.setCursor(0, 15);				// Start at bottom left corner of first text line

	display.printf(" BeeIoT v%s.%s #%i C%i", VMAJOR, VMINOR, bhdb.loopid, bhdb.chcfgid);

	display.setFont(&FreeMonoBold9pt7b);
	display.printf("\n    %s %s\n", bhdb.date, bhdb.time);

	display.drawRect(2, 12+9+9+6, display.width()-(2*2), display.height() - (12+9+9+4 + 9+9), GxEPD_BLACK);

	display.printf("    Gewicht: %s kg\n", String(bhdb.dlog.HiveWeight,3));
	display.printf(" Temp.Beute: %s\n", String(bhdb.dlog.TempHive,2));
	display.printf(" TempExtern: %s\n", String(bhdb.dlog.TempExtern,2));

//	display.printf(" TempIntern: %s\n", String(bhdb.dlog.TempIntern,2));

	display.setFont(&FreeMonoBold9pt7b);	// -> 26chars/line (11 dots/char)
	display.printf(" VBatt:%sV(%s%%) < %sV\n",
			String((float)bhdb.dlog.BattLoad/1000,2),
			String((uint16_t)bhdb.dlog.BattLevel),
			String((float)bhdb.dlog.BattCharge/1000,2));

	display.setFont(&FreeMonoBold9pt7b);	// -> 26 chars/line
    display.print("     Status: ");
	if(BeeIoTStatus == BIOT_SLEEP && ReEntry == 1){
  		display.print(beeiot_StatusString[BIOT_DEEPSLEEP]);
	}else{
  		display.print(beeiot_StatusString[BeeIoTStatus]);
	}

	display.display();				// WakeUp -> update Display

} // end of ShowData()


void showdata29_base(void){
char theader[64];
char ttime[64];
const char tweight[]	="   Gewicht: ";
const char ttemphive[]	="Temp.Beute: ";
const char ttempext[]	="TempExtern:";
const char tbatt[]		="VBatt:";
const char tstatus[]	="Status: ";

	sprintf(theader,	"BeeIoT v%s.%s", VMAJOR, VMINOR);
	sprintf(ttime,		" %s ", bhdb.date);

	display.setRotation(1);    // 1 + 3: print in horizontal format
	display.setTextColor(GxEPD_BLACK);
	display.setFullWindow();

	display.firstPage();
	do	{
		display.fillScreen(GxEPD_WHITE); 	// set the background to white (fill the buffer with value for white)

		display.setFont(&FreeMonoBold12pt7b);   	// -> 22chars/line (13 dots/char)
		display.setCursor(theader_x, theader_y);	// Start at bottom left corner of first text line
		display.print(theader);

		display.setFont(&FreeMonoBold9pt7b);
		display.setCursor(ttime_x, ttime_y);
		display.print(ttime);

//		display.drawRect(tbox_x, tbox_y, display.width()-(2*2), display.height() - (12+9+9+4 + 9+9), GxEPD_BLACK);
		display.drawRect(tbox_x, tbox_y, tbox_wx, tbox_hy, GxEPD_BLACK);

		display.setCursor(tweight_x, tweight_y);
		display.print(tweight);
		display.setCursor(ttemphive_x, ttemphive_y);
		display.print(ttemphive);
		display.setCursor(ttempext_x, ttempext_y);
		display.print(ttempext);

		display.setFont(&FreeMonoBold9pt7b);	// -> 26chars/line (11 dots/char)
		display.setCursor(tbatt_x, tbatt_y);
		display.print(tbatt);

		display.setFont(&FreeMonoBold9pt7b);	// -> 26 chars/line
		display.setCursor(tstatus_x, tstatus_y);
		display.print(tstatus);
	}
	while (display.nextPage());

} // end of ShowData29_base()


void showdata29_updbase(void){
char theader[64];
char ttime[64];
const char tweight[]	="   Gewicht: ";
const char ttemphive[]	="Temp.Beute: ";
const char ttempext[]	="TempExtern:";
const char tbatt[]		="VBatt:";
const char tstatus[]	="Status: ";

	sprintf(theader,	"BeeIoT v%s.%s", VMAJOR, VMINOR);
	sprintf(ttime,		" %s ", bhdb.date);

	display.setRotation(1);    // 1 + 3: print in horizontal format
	display.setTextColor(GxEPD_BLACK);
	display.setPartialWindow(0, 0, display.width(), display.height());

	display.firstPage();
	do	{
		display.fillScreen(GxEPD_WHITE); 	// set the background to white (fill the buffer with value for white)

		display.setFont(&FreeMonoBold12pt7b);   	// -> 22chars/line (13 dots/char)
		display.setCursor(theader_x, theader_y);	// Start at bottom left corner of first text line
		display.print(theader);

		display.setFont(&FreeMonoBold9pt7b);
		display.setCursor(ttime_x, ttime_y);
		display.print(ttime);

//		display.drawRect(tbox_x, tbox_y, display.width()-(2*2), display.height() - (12+9+9+4 + 9+9), GxEPD_BLACK);
		display.drawRect(tbox_x, tbox_y, tbox_wx, tbox_hy, GxEPD_BLACK);

		display.setCursor(tweight_x, tweight_y);
		display.print(tweight);
		display.setCursor(ttemphive_x, ttemphive_y);
		display.print(ttemphive);
		display.setCursor(ttempext_x, ttempext_y);
		display.print(ttempext);

		display.setFont(&FreeMonoBold9pt7b);	// -> 26chars/line (11 dots/char)
		display.setCursor(tbatt_x, tbatt_y);
		display.print(tbatt);

		display.setFont(&FreeMonoBold9pt7b);	// -> 26 chars/line
		display.setCursor(tstatus_x, tstatus_y);
		display.print(tstatus);
	}
	while (display.nextPage());

} // end of ShowData29_updbase()


void showdata29_update(void) {
char dheader[64];
char dtime[64];
char dweight[64];
char dtemphive[64];
char dtempext[64];
char dbatt[64];
char dstatus[64];

// data update fields
const uint16_t updheader_x	= shiftx + (11*15);
const uint16_t updtime_x	= dataupd_x;
const uint16_t updweight_x	= dataupd_x;
const uint16_t updtemphive_x= dataupd_x;
const uint16_t updtempext_x	= dataupd_x;
const uint16_t updbatt_x	= shiftx + (8*10);
const uint16_t updstatus_x	= shiftx + (9*10);

int16_t  tbx, tby;
uint16_t tbw_box, tbh_box;

	sprintf(dheader,	"#%i C%i ", bhdb.loopid, bhdb.chcfgid);
	sprintf(dtime,		"%s  ", bhdb.time);
	sprintf(dweight,	"%s kg ", String(bhdb.dlog.HiveWeight,3));
	sprintf(dtemphive,	"%s ", String(bhdb.dlog.TempHive,2));
	sprintf(dtempext,	"%s ", String(bhdb.dlog.TempExtern,2));
	sprintf(dbatt,		"%sV(%s%%) < %sV ",
				String((float)bhdb.dlog.BattLoad/1000,2),
				String((uint16_t)bhdb.dlog.BattLevel),
				String((float)bhdb.dlog.BattCharge/1000,2));
#ifndef BIoTDBG
	sprintf(dstatus, "%s", (BeeIoTStatus == BIOT_SLEEP) && (ReEntry == 1) ?
				beeiot_StatusString[BIOT_DEEPSLEEP] : beeiot_StatusString[BeeIoTStatus]	);
#else
	sprintf(dstatus, "DEBUGGING" );
#endif

	// show where the update box is and do this outside of the loop
	display.setRotation(1);    // 1 + 3: print in horizontal format
	display.setTextColor(GxEPD_BLACK);

/*
	display.setFont(&FreeMonoBold12pt7b);
	display.getTextBounds(dheader, updheader_x, theader_y, &tbx, &tby, &tbw_box, &tbh_box);
	display.setPartialWindow(tbx, tby, tbw_box, tbh_box);
	display.firstPage();
	do
	{
		display.setCursor(updheader_x, theader_y);
		display.print(dheader);
	}
	while (display.nextPage());
*/
	display.setFont(&FreeMonoBold9pt7b);
	display.getTextBounds(dtime, updtime_x, theader_y, &tbx, &tby, &tbw_box, &tbh_box);
	display.setPartialWindow(tbx, tby, tbw_box, ttempext_y-5);
	display.firstPage();
	do
	{
		display.setFont(&FreeMonoBold12pt7b);
		display.setCursor(updheader_x, theader_y);
		display.print(dheader);

		display.setFont(&FreeMonoBold9pt7b);
		display.setCursor(updtime_x, ttime_y);
		display.print(dtime);
		display.setCursor(updweight_x, tweight_y);
		display.print(dweight);
		display.setCursor(updtemphive_x, ttemphive_y);
		display.print(dtemphive);
		display.setCursor(updtempext_x, ttempext_y);
		display.print(dtempext);
	}
	while (display.nextPage());

	display.setFont(&FreeMonoBold9pt7b);
	display.getTextBounds(dbatt, updbatt_x, tbatt_y, &tbx, &tby, &tbw_box, &tbh_box);
	display.setPartialWindow(tbx, tby, tbw_box, tstatus_y - ttempext_y);
	display.firstPage();
	do
	{
		display.setCursor(updbatt_x, tbatt_y);
		display.print(dbatt);
		display.setCursor(updstatus_x, tstatus_y);
		display.print(dstatus);
	}
	while (display.nextPage());
} // end of showdata29_update()


//***********************************************************
// show Sensor log data on epaper Display
// Input: sampleID= Index on Sensor dataset of BHDB
//***********************************************************
void showbeacon2(void){
	// No EPD panel connected or no Update request pending
	if(isepd==0 || 	EPDupdate==false){
		return;	// no EPD port -> no action
	}

  digitalWrite(EPDGNDEN, LOW);   // enable EPD Ground low side switch

  display.fillScreen(GxEPD_WHITE);
  display.setTextColor(GxEPD_BLACK);
  display.setFont(&FreeMonoBold9pt7b);
  display.setRotation(1);    // 1 + 3: print in horizontal format

  display.setCursor(0, 0);
  display.println();          // adjust cursor to lower left corner of char row

  display.setFont(&FreeMonoBold12pt7b);
  display.printf("BeeIoT.v2   #%i", bhdb.loopid);

  display.setFont(&FreeMonoBold9pt7b);
  display.println();

//  display.setTextColor(GxEPD_RED);
  display.printf("%s %s", bhdb.date, bhdb.time);

  display.setFont(&FreeMonoBold12pt7b);
  display.println();

  display.printf(" Beacon Mode:  \n");

  display.printf("    RSSI: %i\n", bhdb.rssi);
  display.printf("    SNR : %i\n", bhdb.snr);

  display.setFont(&FreeMonoBold9pt7b);	// -> 24chars/line
  display.print("Status : ");
  if(BeeIoTStatus == BIOT_SLEEP && ReEntry == 1){
  		display.print(beeiot_StatusString[BIOT_DEEPSLEEP]);
  }else{
  		display.print(beeiot_StatusString[BeeIoTStatus]);
  }

//  display.writeFastHLine(0, 16, 2, 0xFF); // does not work

  display.display();			// Update Display

  EPDupdate=true;				// EPD update request completed -> reset flag

  // enter EPD low power mode
  digitalWrite(EPDGNDEN, LOW); // disable EPD Ground low side switch
} // end of ShowBeacon()





//**********************************************************************************
// EPD2 Test functions
//**********************************************************************************

void epd2_test(){
  // first update should be full refresh
  helloWorld();
  delay(1000);
  // partial refresh mode can be used to full screen,
  // effective if display panel hasFastPartialUpdate
  helloFullScreenPartialMode();
  delay(1000);
  helloArduino();
  delay(1000);
  helloEpaper();
  delay(1000);
  showFont("FreeMonoBold9pt7b", &FreeMonoBold9pt7b);
  delay(1000);
  // drawBitmaps();
  drawGraphics();
  //return;

if (display.epd2.hasPartialUpdate)
  {
    showPartialUpdate();
    delay(1000);
  } // else // on GDEW0154Z04 only full update available, doesn't look nice
  //drawCornerTest();
  //showBox(16, 16, 48, 32, false);
  //showBox(16, 56, 48, 32, true);
  display.powerOff();
  deepSleepTest();
  return;
}


void helloWorldForDummies()
{
  //Serial.println("helloWorld");
  const char text[] = "Hello World!";
  // most e-papers have width < height (portrait) as native orientation, especially the small ones
  // in GxEPD2 rotation 0 is used for native orientation (most TFT libraries use 0 fix for portrait orientation)
  // set rotation to 1 (rotate right 90 degrees) to have enough space on small displays (landscape)
  display.setRotation(1);
  // select a suitable font in Adafruit_GFX
  display.setFont(&FreeMonoBold9pt7b);
  // on e-papers black on white is more pleasant to read
  display.setTextColor(GxEPD_BLACK);
  // Adafruit_GFX has a handy method getTextBounds() to determine the boundary box for a text for the actual font
  int16_t tbx, tby; uint16_t tbw, tbh; // boundary box window
  display.getTextBounds(text, 0, 0, &tbx, &tby, &tbw, &tbh); // it works for origin 0, 0, fortunately (negative tby!)
  // center bounding box by transposition of origin:
  uint16_t x = ((display.width() - tbw) / 2) - tbx;
  uint16_t y = ((display.height() - tbh) / 2) - tby;
  // full window mode is the initial mode, set it anyway
  display.setFullWindow();
  // here we use paged drawing, even if the processor has enough RAM for full buffer
  // so this can be used with any supported processor board.
  // the cost in code overhead and execution time penalty is marginal
  // tell the graphics class to use paged drawing mode
  display.firstPage();
  do
  {
    // this part of code is executed multiple times, as many as needed,
    // in case of full buffer it is executed once
    // IMPORTANT: each iteration needs to draw the same, to avoid strange effects
    // use a copy of values that might change, don't read e.g. from analog or pins in the loop!
    display.fillScreen(GxEPD_WHITE); // set the background to white (fill the buffer with value for white)
    display.setCursor(x, y); // set the postition to start printing text
    display.print(text); // print some text
    // end of part executed multiple times
  }
  // tell the graphics class to transfer the buffer content (page) to the controller buffer
  // the graphics class will command the controller to refresh to the screen when the last page has been transferred
  // returns true if more pages need be drawn and transferred
  // returns false if the last page has been transferred and the screen refreshed for panels without fast partial update
  // returns false for panels with fast partial update when the controller buffer has been written once more, to make the differential buffers equal
  // (for full buffered with fast partial update the (full) buffer is just transferred again, and false returned)
  while (display.nextPage());
  //Serial.println("helloWorld done");
}

const char HelloWorld[] = "Hello World!";
const char HelloArduino[] = "Hello Arduino!";
const char HelloEpaper[] = "Hello E-Paper!";

void helloWorld()
{
  Serial.println("helloWorld");
  display.setRotation(1);
  display.setFont(&FreeMonoBold9pt7b);
  display.setTextColor(GxEPD_BLACK);
  int16_t tbx, tby; uint16_t tbw, tbh;
  display.getTextBounds(HelloWorld, 0, 0, &tbx, &tby, &tbw, &tbh);
  // center bounding box by transposition of origin:
  uint16_t x = ((display.width() - tbw) / 2) - tbx;
  uint16_t y = ((display.height() - tbh) / 2) - tby;
  display.setFullWindow();
  display.firstPage();
  do
  {
    display.fillScreen(GxEPD_WHITE);
    display.setCursor(x, y);
    display.print(HelloWorld);
  }
  while (display.nextPage());
  //Serial.println("helloWorld done");
}



void helloFullScreenPartialMode()
{
  //Serial.println("helloFullScreenPartialMode");
  const char fullscreen[] = "full screen update";
  const char fpm[] = "fast partial mode";
  const char spm[] = "slow partial mode";
  const char npm[] = "no partial mode";
  display.setPartialWindow(0, 0, display.width(), display.height());
  display.setRotation(1);
  display.setFont(&FreeMonoBold9pt7b);
  display.setTextColor(GxEPD_BLACK);
  const char* updatemode;
  if (display.epd2.hasFastPartialUpdate)
  {
    updatemode = fpm;
  }
  else if (display.epd2.hasPartialUpdate)
  {
    updatemode = spm;
  }
  else
  {
    updatemode = npm;
  }
  // do this outside of the loop
  int16_t tbx, tby; uint16_t tbw, tbh;
  // center full screen text
  display.getTextBounds(fullscreen, 0, 0, &tbx, &tby, &tbw, &tbh);
  uint16_t utx = ((display.width() - tbw) / 2) - tbx;
  uint16_t uty = ((display.height() / 4) - tbh / 2) - tby;
  // center update mode
  display.getTextBounds(updatemode, 0, 0, &tbx, &tby, &tbw, &tbh);
  uint16_t umx = ((display.width() - tbw) / 2) - tbx;
  uint16_t umy = ((display.height() * 3 / 4) - tbh / 2) - tby;
  // center HelloWorld
  display.getTextBounds(HelloWorld, 0, 0, &tbx, &tby, &tbw, &tbh);
  uint16_t hwx = ((display.width() - tbw) / 2) - tbx;
  uint16_t hwy = ((display.height() - tbh) / 2) - tby;
  display.firstPage();
  do
  {
    display.fillScreen(GxEPD_WHITE);
    display.setCursor(hwx, hwy);
    display.print(HelloWorld);
    display.setCursor(utx, uty);
    display.print(fullscreen);
    display.setCursor(umx, umy);
    display.print(updatemode);
  }
  while (display.nextPage());
  //Serial.println("helloFullScreenPartialMode done");
}

void helloArduino()
{
  //Serial.println("helloArduino");
  display.setRotation(1);
  display.setFont(&FreeMonoBold9pt7b);
  display.setTextColor(display.epd2.hasColor ? GxEPD_RED : GxEPD_BLACK);
  int16_t tbx, tby; uint16_t tbw, tbh;
  // align with centered HelloWorld
  display.getTextBounds(HelloWorld, 0, 0, &tbx, &tby, &tbw, &tbh);
  uint16_t x = ((display.width() - tbw) / 2) - tbx;
  // height might be different
  display.getTextBounds(HelloArduino, 0, 0, &tbx, &tby, &tbw, &tbh);
  uint16_t y = ((display.height() / 4) - tbh / 2) - tby; // y is base line!
  // make the window big enough to cover (overwrite) descenders of previous text
  uint16_t wh = FreeMonoBold9pt7b.yAdvance;
  uint16_t wy = (display.height() / 4) - wh / 2;
  display.setPartialWindow(0, wy, display.width(), wh);
  display.firstPage();
  do
  {
    display.fillScreen(GxEPD_WHITE);
    //display.drawRect(x, y - tbh, tbw, tbh, GxEPD_BLACK);
    display.setCursor(x, y);
    display.print(HelloArduino);
  }
  while (display.nextPage());
  delay(1000);
  //Serial.println("helloArduino done");
}

void helloEpaper()
{
  //Serial.println("helloEpaper");
  display.setRotation(1);
  display.setFont(&FreeMonoBold9pt7b);
  display.setTextColor(display.epd2.hasColor ? GxEPD_RED : GxEPD_BLACK);
  int16_t tbx, tby; uint16_t tbw, tbh;
  // align with centered HelloWorld
  display.getTextBounds(HelloWorld, 0, 0, &tbx, &tby, &tbw, &tbh);
  uint16_t x = ((display.width() - tbw) / 2) - tbx;
  // height might be different
  display.getTextBounds(HelloEpaper, 0, 0, &tbx, &tby, &tbw, &tbh);
  uint16_t y = (display.height() * 3 / 4) + tbh / 2; // y is base line!
  // make the window big enough to cover (overwrite) descenders of previous text
  uint16_t wh = FreeMonoBold9pt7b.yAdvance;
  uint16_t wy = (display.height() * 3 / 4) - wh / 2;
  display.setPartialWindow(0, wy, display.width(), wh);
  display.firstPage();
  do
  {
    display.fillScreen(GxEPD_WHITE);
    display.setCursor(x, y);
    display.print(HelloEpaper);
  }
  while (display.nextPage());
  //Serial.println("helloEpaper done");
}

void deepSleepTest()
{
  //Serial.println("deepSleepTest");
  const char hibernating[] = "hibernating ...";
  const char wokeup[] = "woke up";
  const char from[] = "from deep sleep";
  const char again[] = "again";
  display.setRotation(1);
  display.setFont(&FreeMonoBold9pt7b);
  display.setTextColor(GxEPD_BLACK);
  int16_t tbx, tby; uint16_t tbw, tbh;
  // center text
  display.getTextBounds(hibernating, 0, 0, &tbx, &tby, &tbw, &tbh);
  uint16_t x = ((display.width() - tbw) / 2) - tbx;
  uint16_t y = ((display.height() - tbh) / 2) - tby;
  display.setFullWindow();
  display.firstPage();
  do
  {
    display.fillScreen(GxEPD_WHITE);
    display.setCursor(x, y);
    display.print(hibernating);
  }
  while (display.nextPage());
  display.hibernate();
  delay(5000);
  display.getTextBounds(wokeup, 0, 0, &tbx, &tby, &tbw, &tbh);
  uint16_t wx = (display.width() - tbw) / 2;
  uint16_t wy = (display.height() / 3) + tbh / 2; // y is base line!
  display.getTextBounds(from, 0, 0, &tbx, &tby, &tbw, &tbh);
  uint16_t fx = (display.width() - tbw) / 2;
  uint16_t fy = (display.height() * 2 / 3) + tbh / 2; // y is base line!
  display.firstPage();
  do
  {
    display.fillScreen(GxEPD_WHITE);
    display.setCursor(wx, wy);
    display.print(wokeup);
    display.setCursor(fx, fy);
    display.print(from);
  }
  while (display.nextPage());
  delay(5000);
  display.getTextBounds(hibernating, 0, 0, &tbx, &tby, &tbw, &tbh);
  uint16_t hx = (display.width() - tbw) / 2;
  uint16_t hy = (display.height() / 3) + tbh / 2; // y is base line!
  display.getTextBounds(again, 0, 0, &tbx, &tby, &tbw, &tbh);
  uint16_t ax = (display.width() - tbw) / 2;
  uint16_t ay = (display.height() * 2 / 3) + tbh / 2; // y is base line!
  display.firstPage();
  do
  {
    display.fillScreen(GxEPD_WHITE);
    display.setCursor(hx, hy);
    display.print(hibernating);
    display.setCursor(ax, ay);
    display.print(again);
  }
  while (display.nextPage());
  display.hibernate();
  //Serial.println("deepSleepTest done");
}

void showBox(uint16_t x, uint16_t y, uint16_t w, uint16_t h, bool partial)
{
  //Serial.println("showBox");
  display.setRotation(1);
  if (partial)
  {
    display.setPartialWindow(x, y, w, h);
  }
  else
  {
    display.setFullWindow();
  }
  display.firstPage();
  do
  {
    display.fillScreen(GxEPD_WHITE);
    display.fillRect(x, y, w, h, GxEPD_BLACK);
  }
  while (display.nextPage());
  //Serial.println("showBox done");
}

void drawCornerTest()
{
  display.setFullWindow();
  display.setFont(&FreeMonoBold9pt7b);
  display.setTextColor(GxEPD_BLACK);
  for (uint16_t r = 0; r <= 4; r++)
  {
    display.setRotation(r);
    display.firstPage();
    do
    {
      display.fillScreen(GxEPD_WHITE);
      display.fillRect(0, 0, 8, 8, GxEPD_BLACK);
      display.fillRect(display.width() - 18, 0, 16, 16, GxEPD_BLACK);
      display.fillRect(display.width() - 25, display.height() - 25, 24, 24, GxEPD_BLACK);
      display.fillRect(0, display.height() - 33, 32, 32, GxEPD_BLACK);
      display.setCursor(display.width() / 2, display.height() / 2);
      display.print(display.getRotation());
    }
    while (display.nextPage());
    delay(2000);
  }
}

void showFont(const char name[], const GFXfont* f)
{
  display.setFullWindow();
  display.setRotation(0);
  display.setTextColor(GxEPD_BLACK);
  display.firstPage();
  do
  {
    drawFont(name, f);
  }
  while (display.nextPage());
}

void drawFont(const char name[], const GFXfont* f)
{
  //display.setRotation(0);
  display.fillScreen(GxEPD_WHITE);
  display.setTextColor(GxEPD_BLACK);
  display.setFont(f);
  display.setCursor(0, 0);
  display.println();
  display.println(name);
  display.println(" !\"#$%&'()*+,-./");
  display.println("0123456789:;<=>?");
  display.println("@ABCDEFGHIJKLMNO");
  display.println("PQRSTUVWXYZ[\\]^_");
  if (display.epd2.hasColor)
  {
    display.setTextColor(GxEPD_RED);
  }
  display.println("`abcdefghijklmno");
  display.println("pqrstuvwxyz{|}~ ");
}

// note for partial update window and setPartialWindow() method:
// partial update window size and position is on byte boundary in physical x direction
// the size is increased in setPartialWindow() if x or w are not multiple of 8 for even rotation, y or h for odd rotation
// see also comment in GxEPD2_BW.h, GxEPD2_3C.h or GxEPD2_GFX.h for method setPartialWindow()
// showPartialUpdate() purposely uses values that are not multiples of 8 to test this

void showPartialUpdate()
{
  // some useful background
//	helloWorld();
   // use asymmetric values for test
  uint16_t box_x = 10;
  uint16_t box_y = 15;
  uint16_t box_w = 70;
  uint16_t box_h = 20;
  uint16_t cursor_y = box_y + box_h - 6;
  float value = 13.95;
  uint16_t incr = display.epd2.hasFastPartialUpdate ? 1 : 3;
  display.setFont(&FreeMonoBold9pt7b);
  display.setTextColor(GxEPD_BLACK);
  // show where the update box is
  for (uint16_t r = 0; r < 4; r++)
  {
    display.setRotation(r);
    display.setPartialWindow(box_x, box_y, box_w, box_h);
    display.firstPage();
    do
    {
      display.fillRect(box_x, box_y, box_w, box_h, GxEPD_BLACK);
      //display.fillScreen(GxEPD_BLACK);
    }
    while (display.nextPage());
    delay(2000);
    display.firstPage();
    do
    {
      display.fillRect(box_x, box_y, box_w, box_h, GxEPD_WHITE);
    }
    while (display.nextPage());
    delay(1000);
  }
  return;
  // show updates in the update box
  for (uint16_t r = 0; r < 4; r++)
  {
    display.setRotation(r);
    display.setPartialWindow(box_x, box_y, box_w, box_h);
    for (uint16_t i = 1; i <= 10; i += incr)
    {
      display.firstPage();
      do
      {
        display.fillRect(box_x, box_y, box_w, box_h, GxEPD_WHITE);
        display.setCursor(box_x, cursor_y);
        display.print(value * i, 2);
      }
      while (display.nextPage());
      delay(500);
    }
    delay(1000);
    display.firstPage();
    do
    {
      display.fillRect(box_x, box_y, box_w, box_h, GxEPD_WHITE);
    }
    while (display.nextPage());
    delay(1000);
  }
}




void drawGraphics()
{
  display.setRotation(0);
  display.firstPage();
  do
  {
    display.drawRect(display.width() / 8, display.height() / 8, display.width() * 3 / 4, display.height() * 3 / 4, GxEPD_BLACK);
    display.drawLine(display.width() / 8, display.height() / 8, display.width() * 7 / 8, display.height() * 7 / 8, GxEPD_BLACK);
    display.drawLine(display.width() / 8, display.height() * 7 / 8, display.width() * 7 / 8, display.height() / 8, GxEPD_BLACK);
    display.drawCircle(display.width() / 2, display.height() / 2, display.height() / 4, GxEPD_BLACK);
    display.drawPixel(display.width() / 4, display.height() / 2 , GxEPD_BLACK);
    display.drawPixel(display.width() * 3 / 4, display.height() / 2 , GxEPD_BLACK);
  }
  while (display.nextPage());
}


#endif // EPD2_CONFIG
