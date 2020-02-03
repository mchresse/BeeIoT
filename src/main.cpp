//*******************************************************************
// BeeIoT - Main Header file  
// from Project https://github.com/mchresse/BeeIoT
//
// Description:
// Contains main setup() and loop() routines for esspressif32 platforms.
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
// - The "espressif/arduino-esp32/preferences" library, 
//   distributed under the Apache License, Version 2.0
// - MQTT Library distributed under the MIT-License: 
//   https://opensource.org/licenses/mit-license.php
// - 
//
//
//*******************************************************************
// BeeIoT Local Libraries
//*******************************************************************
// For ESP32-DevKitC PIN Configuration look at BeeIoT.h

#include <Arduino.h>
#include <stdio.h>
#include "sys/types.h"
#include <iostream>
#include <string>
// #include <esp_log.h> 
// from Espressif Systems IDF: https://github.com/espressif/esp-idf/tree/71b4768df8091a6e6d6ad3b5c2f09a058f271348/components/log

#include "sdkconfig.h"   // generated from Arduino IDE
#include <Preferences.h> // from espressif-esp32 library @ GitHub
// see https://github.com/espressif/arduino-esp32/blob/master/libraries/Preferences/examples/StartCounter/StartCounter.ino

// Libraries for SD card at ESP32_
// ...has support for FAT32 support with long filenames
#include <SPI.h>        // from Arduino IDE
#include <FS.h>         // from Arduino IDE
#include <SD.h>         // from Arduino IDE
#include "sdcard.h"

// Libs for WaveShare ePaper 2.7 inch r/w/b Pinning GxGDEW027C44
#include <GxEPD.h>                      // from ZinggJM/GxEPD (https://github.com/ZinggJM/GxEPD)
#include <GxGDEW027C44/GxGDEW027C44.h>  // 2.7" b/w/r
#include "BitmapWaveShare.h"            // from WaveShare -> FreeWare

// FreeFonts from Adafruit_GFX          // from adafruit / Adafruit-GFX-Library  (BSD license)
#include <Fonts/FreeMonoBold9pt7b.h>    
#include <Fonts/FreeMonoBold12pt7b.h>
//#include <Fonts/FreeMonoBold18pt7b.h>
//#include <Fonts/FreeMonoBold24pt7b.h>
//#include <Fonts/FreeSansBold24pt7b.h>

// DS18B20 libraries
#include <OneWire.h>            // from PaulStoffregen/OneWire library @ GitHub
#include <DallasTemperature.h>  // LGPL v2.1
#include "owbus.h"

// ADS1115 I2C Library
#include <driver/i2c.h>         // from esp-idf/components/driver/I2C.h library @ GitHub

// Libraries for WIFI & to get time from NTP Server
#include <WiFi.h>               // from espressif-esp32 library @ GitHub
#include "wificfg.h"            // local
#include "RTClib.h"             // from by JeeLabs adafruit /RTClib library @ GitHub

// include TCP Client library
#include "TCPClient.h"          // local

// include MQTT Client library
#include <MQTTClient.h>         // local

// Library fo HX711 Access
#include <HX711.h>              // HX711 library for Arduino (https://github.com/bogde/HX711)
#include "HX711Scale.h"         // local

#include <LoRa.h>       // Lora Lib from SanDeep (https://github.com/sandeepmistry/arduino-LoRa)
#include "BeeIoTWan.h"
#include "beelora.h"            // local: Lora Radio settings and BeeIoT WAN protocol definitions
#include "beeiot.h"             // local: provides all GPIO PIN configurations of all sensor Ports !

//************************************
// Global data object declarations
//************************************

#define LOOPTIME    600		// [sec] Loop wait time: 60 for 1 Minute
#define SLEEPTIME   3E7		// Mikrosekunden hier 3s

// Save reading number on RTC memory
// RTC_DATA_ATTR int loopID = 0;           // generic loop counter

// Define deep sleep options
uint64_t uS_TO_S_FACTOR = 1000000;  // Conversion factor for micro seconds to seconds
// Sleep for 10 minutes = 600 seconds
uint64_t TIME_TO_SLEEP = 600;

// Central Database of all measured values and runtime parameters
dataset		bhdb;
unsigned int	lflags;               // BeeIoT log flag field
Preferences preferences;        // we must generate this object of the preference library

// 8 configuration values max managed by webpage
#define CONFIGSETS    8
String ConfigName[CONFIGSETS];     // name of the configuration value
String ConfigValue[CONFIGSETS];    // the value itself (String)
int    ConfigStatus[CONFIGSETS];   // status of the value    0 = not set    1 = valid   -1 = not valid
int    ConfigType[CONFIGSETS];     // type of the value    0 = not set    1 = String (textbox)   2 = Byte (slider)

extern int iswifi;              // =0 WIFI flag o.k.
extern int isntp;               // =0 bhdb has latst timestamp
extern int isrtc;               // =0 RTC Time discovered
extern int issdcard;            // =0 SDCard found flag o.k.
extern int isepd;               // =0 ePaper found
extern int islora;              // =1 LoRa client is active

extern GxEPD_Class  display;    // ePaper instance from MultiSPI Module
extern HX711        scale;      // managed in HX711Scale module
extern i2c_config_t conf;       // ESP IDF I2C port configuration object
extern RTC_DS3231   rtc;        // Create RTC Instance

// construct the object attTCPClient of class TCPClient
TCPClient attTCPClient = TCPClient();

// construct the object attMQTTClient of class MQTTClient
MQTTClient attMQTTClient = MQTTClient();
byte MQTTClient_Connected;
byte CounterForMQTT;

extern String WebRequestHostAddress;     // global variable used to store Server IP-Address of HTTP-Request
extern byte   RouterNetworkDeviceState;

// LoRa protocol frequence parameter
long lastSendTime = 0;        // last send time
int LoRa_interval = 2000;     // interval between sends
char LoRaBuffer[256];         // buffer for LoRa Packages
extern byte msgCount;


//************************************
// Global function declarations
//************************************
void showFont(const char name[], const GFXfont* f);
void showFontCallback(void);
void showPartialUpdate(float data);
void ProcessAndValidateConfigValues(int countValues);
void InitConfig();
void CheckWebPage();

//*******************************************************************
// BeeIoT Setup Routine
//
//*******************************************************************
void setup() {
// lflags = 0;   // Define Log level (search for Log values in beeiot.h)
// lflags = LOGBH + LOGOW + LOGHX + LOGLAN + LOGEPD + LOGSD + LOGADS + LOGSPI + LOGLORAR + LOGLORAW;
lflags = LOGBH + LOGLORAW +LOGLORAR;

  // put your setup code here, to run once:
  pinMode(LED_RED,   OUTPUT); 
  digitalWrite(LED_RED, LOW);     // signal Setup Phase


  while(!Serial);                 // wait to connect to computer
  Serial.begin(115200);           // enable Ser. Monitor Baud rate
  delay(3000);                    // wait for console opening

  Serial.println();
  Serial.println(">*******************************<");
  Serial.println("> BeeIoT - BeeHive Weight Scale <");
  Serial.println(">       by R.Esser (c) 10/2019  <");
  Serial.println(">*******************************<");
  if(lflags > 0)
    Serial.printf ("LogLevel: %i\n", lflags);
  
  BHLOG(LOGBH)Serial.println("Start Sensor Setup Phase ...");

//***************************************************************
// esp_sleep_wakeup_cause_t wakeup_cause; // Variable für wakeup Ursache
// setenv("TZ", TZ_INFO, 1);             // Zeitzone  muss nach dem reset neu eingestellt werden
// tzset();
// wakeup_cause = esp_sleep_get_wakeup_cause(); // wakeup Ursache holen
// if (wakeup_cause != 3) 
//     ErsterStart();     // Wenn wakeup durch Reset -> start initial setup code

//***************************************************************
// get Board Chip ID (WiFI MAC)
  bhdb.BoardID=ESP.getEfuseMac();//The chip ID is essentially its MAC address(length: 6 bytes).
	BHLOG(LOGBH) Serial.printf("  Setup: ESP32 DevKitC Chip ID = %04X",
      (uint16_t)(bhdb.BoardID>>32));                            //print High 2 bytes
  BHLOG(LOGBH) Serial.printf("%08X\n",(uint32_t)bhdb.BoardID);  //print Low 4bytes.

//***************************************************************
// Preset BeeIoT runtime config values
  BHLOG(LOGBH) Serial.println("  Setup: Init runtime config settings");
  InitConfig();
  
//***************************************************************
  BHLOG(LOGBH) Serial.println("  Setup: Init RTC Module DS3231");
  if (setup_rtc() != 0){
    BHLOG(LOGBH) Serial.println("  Setup: RTC setup failed");
    // enter exit code here, if needed (monitoring is hard without correct timestamp)
    // isrtc should be -1 here; hopefully NTP can help out later on
  }else{
    BHLOG(LOGLAN) rtc_test();
  }
  
//***************************************************************
  BHLOG(LOGBH) Serial.println("  Setup: SPI Devices ...");
  if (setup_spi_VSPI() != 0){ 
    BHLOG(LOGBH) Serial.println("  Setup: SPI setup failed");
    // enter here exit code, if needed
  }

//***************************************************************
  BHLOG(LOGBH) Serial.println("  Setup: HX711 Weight Cell");
  if (setup_hx711Scale() != 0){
    BHLOG(LOGBH) Serial.println("  Setup: HX711 Weight Cell setup failed");
    // enter here exit code, if needed
  }

//***************************************************************
  BHLOG(LOGBH) Serial.println("  Setup: ADS11x5");
  if (setup_i2c_ADS() != 0){
    BHLOG(LOGBH) Serial.println("  Setup: ADS11x5 setup failed");
    // enter here exit code, if needed
  }

//***************************************************************
// setup Wifi & NTP & RTC time & Web service
  BHLOG(LOGBH) Serial.println("  Setup: Wifi in Station Mode");
  if (setup_wifi() != 0){
    BHLOG(LOGBH) Serial.println("  Setup: Wifi setup failed");
    // enter here exit code, if needed
    BHLOG(LOGBH) Serial.println("  Setup: No WIFI no NTP-Time.");
    // probably we are LOOPTIME ahead ?!
    // recalc bhdb "timestamp+LOOPTIME"  here
  }else{  // WIFI connected  
    if (setup_ntp() != 0){
      BHLOG(LOGBH) Serial.println("  Setup: NTP setup failed");
      isntp = -1;
      // enter exit code here, if needed
      // NTP requires connected WIFI ! -> check iswifi also
    }else{
      BHLOG(LOGLAN) Serial.println("  Setup: Get new Date & Time:");
      ntp2rtc();       // init RTC time once at restart
    }

// start the webserver to listen for request of clients (in LAN or own ESP32 network)
//    BHLOG(LOGBH) Serial.println("  Setup: Start Webserver");
//    Webserver_Start();    
  }

  getTimeStamp();  // get curr. time either by NTP or RTC -> update bhdb

//***************************************************************
  BHLOG(LOGBH) Serial.println("  Setup: SD Card");
  if (setup_sd() != 0){
    BHLOG(LOGBH) Serial.println("  Setup: SD Card Setup failed");
    // enter exit code here, if needed
  }

//***************************************************************
  BHLOG(LOGBH) Serial.println("  Setup: LoRa SPI device & Base layer");
  if (!setup_LoRa()){
    BHLOG(LOGBH) Serial.println("  Setup: LoRa Base layer setup failed");
    // enter exit code here, if needed
  }

//***************************************************************
  BHLOG(LOGBH) Serial.println("  Setup: OneWire Bus setup");
  if (setup_owbus() == 0){
    BHLOG(LOGBH) Serial.println("  Setup: No OneWire devices found");
    // enter exit code here, if needed
  }
  GetOWsensor(0); // read temperature the first time

//***************************************************************
  BHLOG(LOGBH)Serial.println("  Setup: ePaper + show start frame ");
  // isepd=-1; // Disable EPD for test purpose only

  if (setup_epd() != 0){
    BHLOG(LOGBH)Serial.println("  Setup: ePaper Test failed");
    // enter exit code here, if needed
  }

//***************************************************************
  BHLOG(LOGBH) Serial.println("Setup Phase done");
  Serial.println(" ");

// while(1);  // for test purpose

} // end of BeeIoT setup()



//*******************************************************************
// BeeIoT Main Routine: Loop()
// endless loop
//*******************************************************************
void loop() {

  digitalWrite(LED_RED, HIGH);  // show we have reachd loop() phase
  BHLOG(LOGBH) Serial.println(">*******************************************<");
  BHLOG(LOGBH) Serial.println("> Start next BeeIoT Weight Scale loop"); 
  BHLOG(LOGBH) Serial.printf ("> Loop# %i  (Laps: %i, BHDB[%i])\n", 
                  bhdb.loopid + (bhdb.laps*datasetsize), bhdb.laps, bhdb.loopid);
  BHLOG(LOGBH) Serial.println(">*******************************************<");

  bhdb.dlog[bhdb.loopid].index = bhdb.loopid + (bhdb.laps*datasetsize);
  strncpy(bhdb.dlog[bhdb.loopid].comment, "o.k.", 5);

//***************************************************************
// Check for Web Config page update
#ifdef WIFI_CONFIG
  if(iswifi == 0){
//      BHLOG(LOGLAN) Serial.println("  Loop: Check for new WebPage Client request...");
//      CheckWebPage();
  }
#endif // WIFI_CONFIG


//***************************************************************
// get current time to bhdb
  if(getTimeStamp() == -2){   // no valid time source found
    strncpy(bhdb.dlog[bhdb.loopid].timeStamp, "0000-00-00 00:00:00", LENTMSTAMP);
  }else{
    // store curr. timstamp for next data row
    sprintf(bhdb.dlog[bhdb.loopid].timeStamp, "%s %s", bhdb.date, bhdb.time);
  }

//***************************************************************
#ifdef HX711_CONFIG
  float weight;
  scale.power_up();  // HX711 WakeUp Device
 
  // Acquire raw reading
  weight = HX711_read(0);
  BHLOG(LOGHX) Serial.printf("  Loop: Weight(raw) : %d", (u_int) weight);
  
  // Acquire unit reading
  weight = HX711_read(1);
  BHLOG(LOGHX) Serial.printf(" - Weight(unit): %.3f kg\n", weight);
  bhdb.dlog[bhdb.loopid].HiveWeight = weight;

  scale.power_down();
#endif // HX711_CONFIG

//***************************************************************
#ifdef ONEWIRE_CONFIG
  setup_owbus();
  GetOWsensor(bhdb.loopid);   // Get all temp values directly into bhdb

  while(bhdb.dlog[bhdb.loopid].TempHive > 70){  // if we have just started lets do it again to get right values
    GetOWsensor(bhdb.loopid);                   // Get all temp values directly into bhdb
    delay(500);
  }
#endif // ONEWIRE_CONFIG

//***************************************************************
#ifdef ADS_CONFIG
int16_t addata = 0;   // raw ADS Data buffer
float x;              // Volt calculation buffer

  // read out all ADS channels 0..3
  BHLOG(LOGADS) Serial.print("  Loop: ADSPort(0-3): ");
  addata = ads_read(0);         // get 3.3V line value of ESP32 devKit in mV
  bhdb.dlog[bhdb.loopid].ESP3V = addata;
  BHLOG(LOGADS) Serial.print((float)addata/1000, 2);
  BHLOG(LOGADS) Serial.print("V - ");

  addata = ads_read(1);         // get Level of Battery Charge Input from Ext-USB port
  bhdb.dlog[bhdb.loopid].BattCharge = addata*2; //  measured: 5V/2 = 2,5V value
  BHLOG(LOGADS) Serial.print((float)addata*2/1000, 2);
  BHLOG(LOGADS) Serial.print("V - ");

  addata = ads_read(2);         // Get Battery raw Capacity in Volt ((10%) 3.7V - 4.2V(100%))
  // calculate Battery Load Level in %
  x = ((float)(addata-BATTERY_MIN_LEVEL)/
       (float)(BATTERY_MAX_LEVEL-BATTERY_MIN_LEVEL) )* 100;
  bhdb.dlog[bhdb.loopid].BattLevel = (int16_t) x;
  bhdb.dlog[bhdb.loopid].BattLoad = addata;
  BHLOG(LOGADS) Serial.printf("%.2fV (%i%%)", (float)addata/1000, bhdb.dlog[bhdb.loopid].BattLevel);
  BHLOG(LOGADS) Serial.print(" - ");

  addata = ads_read(3);         // get level of 5V Power line of Extension board (not of ESP32 DevKit !)
  bhdb.dlog[bhdb.loopid].Board5V = addata*2; //  measured: 5V/2 = 2,5V value
  BHLOG(LOGADS) Serial.print((float)addata/1000, 2);
  BHLOG(LOGADS) Serial.println("V");
#endif // ADS_CONFIG

//***************************************************************
  Logdata();      // save all collected sensor data to SD or send by LoRa

//***************************************************************
#ifdef EPD_CONFIG
    BHLOG(LOGEPD) Serial.println("  Loop: Show Sensor Data on EPD");
    showdata(bhdb.loopid);
#endif

//***************************************************************
// End of Main Loop ->  Save data and enter Sleep/Delay Mode
  digitalWrite(LED_RED, HIGH);
  bhdb.loopid++;     // Increment LoopID each time
  if (bhdb.loopid == datasetsize){  //Max. numbers of datarows filled ?
    bhdb.loopid = 0;  // reset datarow idx -> round robin buffer for 1 day only
    bhdb.laps++;      // remember how many times we had a bhdb bufferoverflow.
  }

//***************************************************************
  BHLOG(LOGBH) Serial.printf("  Loop: Enter Sleep/Wait Mode for %i sec.\n", LOOPTIME);
  // Start deep sleep
  //  esp_sleep_enable_timer_wakeup(SLEEPTIME);    // Deep Sleep Zeit einstellen
  //  esp_deep_sleep_start();                       // Starte Deep Sleep
  // or
  mydelay(LOOPTIME*1000);   // wait with blinking red LED

  BHLOG(LOGBH) Serial.println();
} // end of loop()



//*******************************************************************
// Logdata()
// Append current sensor data set to SD card -> logdata file
// and send it via LoRaWAN
//*******************************************************************
void Logdata(void) {
uint16_t sample;
String dataMessage; // Global data objects

  sample = (bhdb.laps*datasetsize) + bhdb.loopid;

// Create tatus Report based on the sensor readings
  dataMessage =  
              String(bhdb.date) + " " + 
              String(bhdb.time) + "," + 
              String(bhdb.dlog[bhdb.loopid].HiveWeight) + "," +
              String(bhdb.dlog[bhdb.loopid].TempExtern) + "," +
              String(bhdb.dlog[bhdb.loopid].TempIntern) + "," +
              String(bhdb.dlog[bhdb.loopid].TempHive)   + "," +
              String(bhdb.dlog[bhdb.loopid].TempRTC)    + "," +
              String((float)bhdb.dlog[bhdb.loopid].ESP3V/1000)      + "," +
              String((float)bhdb.dlog[bhdb.loopid].Board5V/1000)    + "," +
              String((float)bhdb.dlog[bhdb.loopid].BattCharge/1000) + "," +
              String((float)bhdb.dlog[bhdb.loopid].BattLoad/1000)   + "," +
              String(bhdb.dlog[bhdb.loopid].BattLevel)  + "#" +
              String(sample) + " " +
              String(bhdb.dlog[bhdb.loopid].comment) +
              "\r\n";       // OS common EOL: 0D0A
  Serial.printf("  Loop[%i]: ", sample);
  Serial.print(dataMessage);

  // Write the sensor readings on the SD card
  if(issdcard ==0){
    appendFile(SD, SDLOGPATH, dataMessage.c_str());
  }else{
    BHLOG(LOGSD) Serial.println("  Log: No SDCard, no local Logfile...");
  }

  // Send Sensor report via BeeIoT-LoRa ...
  if(islora){  // do we have an active connection (are we joined ?)
    LoRaLog(CMD_LOGSTATUS, (char *) dataMessage.c_str(), (byte)dataMessage.length(), 0); // in sync mode
    }else{
    BHLOG(LOGLORAW) Serial.println("  Log: No LoRa, no BeeIoTWAN on air ...");
  }

  return;
}

void mydelay(int32_t tval){
  int fblink = tval / 1000;   // get # of seconds == blink frequence
  int i;
  for (i=0; i < fblink/2; i++){
    digitalWrite(LED_RED, LOW);
//      if(iswifi == 0){
//        CheckWebPage();
//      }
    delay(250);  // wait 0.25 second
    digitalWrite(LED_RED, HIGH);
//      if(iswifi == 0){
//        CheckWebPage();
//      }
    delay(2000);  // wait 2 second
  }
}

//*******************************************************************
// this section handles configuration values which can be configured 
// via webpage form in a webbrowser
//*******************************************************************
// Initalize the values 
void InitConfig(){
  int i;

  bhdb.loopid       = 0;
  bhdb.laps         = 0;
  bhdb.formattedDate[0] = 0;
  bhdb.date[0]      = 0;
  bhdb.time[0]      = 0;
  bhdb.ipaddr[0]    = 0;
  // bhdb.BoardID      = 0;  already defined
  for(i=0; i<datasetsize;i++){
    bhdb.dlog[i].index       =0;
    bhdb.dlog[i].HiveWeight  =0;
    bhdb.dlog[i].TempExtern  =0;
    bhdb.dlog[i].TempIntern  =0;
    bhdb.dlog[i].TempHive    =0;
    bhdb.dlog[i].TempRTC     =0;
    bhdb.dlog[i].timeStamp[0]=0;
    bhdb.dlog[i].ESP3V       =0;
    bhdb.dlog[i].Board5V     =0;
    bhdb.dlog[i].BattLevel   =0;
    bhdb.dlog[i].BattCharge  =0;
    bhdb.dlog[i].BattLoad    =0;
    strncpy(bhdb.dlog[i].comment, "OK", 3);
  }
  
  // see https://github.com/espressif/arduino-esp32/blob/master/libraries/Preferences/examples/StartCounter/StartCounter.ino 
  preferences.begin("MyJourney", false);

  // takeout 4 Strings out of the Non-volatile storage
  String strSSID        = preferences.getString("SSID", "");
  String strPassword    = preferences.getString("Password", "");
  String strDeviceID    = preferences.getString("DeviceID", "");
  String strDeviceToken = preferences.getString("DeviceToken", "");

  // reset all default config parameter sets
  for (int count = 0; count < CONFIGSETS; count++){
    ConfigName[count]   = "";
    ConfigValue[count]  = "";
    ConfigStatus[count] = 0;
    ConfigType[count]   = 0;
  }

  // initialize config set values and set names
  ConfigName[0] = "LED";
  ConfigType[0] = 1;     // type textbox
  
  ConfigName[1] = "SSID";
  ConfigType[1] = 1;
  
  ConfigName[2] = "PASSWORD";
  ConfigType[2] = 1;
  
  ConfigName[3] = "DEVICE ID";
  ConfigType[3] = 1;         
  
  ConfigName[4] = "DEVICE TOKEN";
  ConfigType[4] = 1;
  
  // put the NVS stored values in RAM for the program
  ConfigValue[1] = strSSID;
  ConfigValue[2] = strPassword;
  ConfigValue[3] = strDeviceID;
  ConfigValue[4] = strDeviceToken;

  MQTTClient_Connected = false;
  CounterForMQTT = 0;
}

// check the ConfigValues and set ConfigStatus
// process the first ConfigValue to switch something like LED=ON/OFF
void ProcessAndValidateConfigValues(int countValues){
  BHLOG(LOGLAN) Serial.printf("  Webserver: ProcessConfigValues(%i)\n", countValues);
  if (countValues > CONFIGSETS) {
    countValues = CONFIGSETS;
  }

  // store the second to fifth value in non-volatile storage
  if (countValues > 4){
    preferences.putString("SSID",        ConfigValue[1]);
    preferences.putString("Password",    ConfigValue[2]);
    preferences.putString("DeviceID",    ConfigValue[3]);
    preferences.putString("DeviceToken", ConfigValue[4]);
  }

  // in our application the first value must be "00" or "FF" (as text string)
  if ((ConfigValue[0].equals("00")) || (ConfigValue[0].equals("FF"))){
    ConfigStatus[0] = 1;    // Value is valid
    BHLOG(LOGLAN) Serial.printf("  Webserver: Processing command: %s\n", ConfigValue[0].c_str());
  }else{
    ConfigStatus[0] = -1;   // Value is not valid
    BHLOG(LOGLAN) Serial.printf("  Webserver: Wrong command: %s\n", ConfigValue[0].c_str());
  }

  // processing command from here:
  // first config value is used to switch LED ( = Actor)
  if (ConfigValue[0].equals("00")){
    digitalWrite(LED_RED, HIGH);
    // do something with actors: SwitchActor(ACTOR_OFF);
  } else if (ConfigValue[0].equals("FF")){
    digitalWrite(LED_RED, LOW);
    // do something other with actors: SwitchActor(ACTOR_ON);
  }  
}

//*******************************************************************
// CheckWebPage()
// check web space for client request
// in case we are connected send a MQTT response
//*******************************************************************
void CheckWebPage(){
  int i;
  String GETParameter = Webserver_GetRequestGETParameter();   // look for client request
  
    if (GETParameter.length() > 0){        // we got a request, client connection stays open
      BHLOG(LOGLAN) i = GETParameter.length();
      BHLOG(LOGLAN) Serial.printf("  CheckWebPage: GETParameter[%i]=", i);
      BHLOG(LOGLAN) Serial.println(GETParameter);
      if (GETParameter.length() > 1){      // request contains some GET parameter
          
          // decode the GET parameter and set ConfigValues
          int countValues = DecodeGETParameterAndSetConfigValues(GETParameter);
          BHLOG(LOGLAN) Serial.printf("  CheckWebPage: Interpreting <%i> parameters for %s\n", countValues, WebRequestHostAddress.c_str());
          
          // the user entered this address in browser, with GET parameter values for configuration
          // default: if (WebRequestHostAddress == "192.168.4.1"){
          if (WebRequestHostAddress == bhdb.ipaddr){
                // check and process 5 ConfigValues, switch the LED, 
                // store SSID, Password, DeviceID and DeviceToken in non-volatile storage
                ProcessAndValidateConfigValues(5); 

                // takeout SSID and Password out of non-volatile storage  
                String strSSID = preferences.getString("SSID", "");
                String strPassword = preferences.getString("Password", "");

                // convert to char*: https://coderwall.com/p/zfmwsg/arduino-string-to-char
                char* txtSSID = const_cast<char*>(strSSID.c_str()); 
                char* txtPassword = const_cast<char*>(strPassword.c_str());
                BHLOG(LOGLAN) Serial.printf("  WebServer: Reconnect SSID:%s - PWD:%s\n", txtSSID, txtPassword);

                // disconnect from router network
//                int successDisconnect = wifi_disconnect();
                delay(1000);  // wait a second
                
                // then try to connect once new with new login-data
//                int successConnect = wifi_connect(txtSSID, txtPassword, HOSTNAME);
                  int successConnect =1; // shortcut for test purpose

                if (successConnect == 1){
                    iswifi = 0;
                    RouterNetworkDeviceState = NETWORKSTATE_LOGGED;                 // set the state

                    CounterForMQTT = 0;
                }else{
                    RouterNetworkDeviceState = NETWORKSTATE_NOTLOGGED;              // set the state                 
                }   
                MQTTClient_Connected = false;
          }
      }
      String HTMLPage;

      // the user entered this address in the browser, to get the configuration webpage               
      // default: if (WebRequestHostAddress == "192.168.4.1"){
      if (WebRequestHostAddress == bhdb.ipaddr){
          // build a new Configuration webpage with form and new ConfigValues entered in textboxes or represented by sliders
          HTMLPage = EncodeFormHTMLFromValues("BeeIoT CONFIG Page", 5) + 
                "<br>IP Address  : " + WiFi_GetOwnIPAddressInRouterNetwork() +
                "<br>Battery Load:";
      }
      Webserver_SendHTMLPage(HTMLPage);    // send out the webpage to client = webbrowser and close client connection
    }

/* by now no MQTT account
  if (iswifi == 0){             // if we are logged in
    CounterForMQTT++; 
    if (CounterForMQTT > 40){   // do this approx every 2 seconds
      CounterForMQTT = 0;      
      // not yet connected
      if (MQTTClient_Connected == false){
        // try to connect to AllThingsTalk MQTT Broker and switch RGB-LEDs
        int NodeState = ConnectToATT();     
        if (NodeState > 0){   //  connection and subscription established successfully
          MQTTClient_Connected = true;            // we can go in normal mode 
        }
      }
        
      // we successfully connected with MQTT broker and subscribed to topic and can go in normal mode
      if (MQTTClient_Connected == true){           
        // generate virtual sensor data (10..40)
        CurrentSensorDate++;
        if (CurrentSensorDate > 40) {CurrentSensorDate = 10;};
    
        // Send sensor data via MQTT to AllThingsTalk
        SendSensorDateToATT("temperature", CurrentSensorDate);      

        // read the voltage at pin 36, divide by 64
        int analog_value = analogRead(PIN_SENSOR) >> 6;

        // Send sensor data via MQTT to AllThingsTalk
        SendSensorDateToATT("light", analog_value); 
      }
    }
  }
*/
} // end of CheckWebPage()

// end of BeeIoT main