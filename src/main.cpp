//*******************************************************************
// BeeIoT - Main Header file
// Contains main setup() and loop() routines for esspressif32 platforms.
//*******************************************************************
// For ESP32-DevKitC PIN Configuration look at BeeIoT.h

//*******************************************************************
// BeeIoT Local Libraries
//*******************************************************************

#include <Arduino.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <esp_log.h>
#include "sdkconfig.h"
#include <Preferences.h>
// see https://github.com/espressif/arduino-esp32/blob/master/libraries/Preferences/examples/StartCounter/StartCounter.ino

// Libraries for SD card at ESP32_
// ...has support for FAT32 support with long filenames
#include <SPI.h>
#include "FS.h"
#include "SD.h"
#include "sdcard.h"
#include <LoRa.h>

// Libs for WaveShare ePaper 2.7 inch r/w/b Pinning GxGDEW027C44
#include <GxEPD.h>
#include <GxGDEW027C44/GxGDEW027C44.h>  // 2.7" b/w/r
// #include "BitmapGraphics.h"
// #include "BitmapExamples.h"
#include "BitmapWaveShare.h"

// FreeFonts from Adafruit_GFX
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>
//#include <Fonts/FreeMonoBold18pt7b.h>
//#include <Fonts/FreeMonoBold24pt7b.h>
//#include <Fonts/FreeSansBold24pt7b.h>

// DS18B20 libraries
#include <OneWire.h>
#include <DallasTemperature.h>
#include "owbus.h"

// ADS1115 I2C Library
#include <driver/i2c.h>

// Libraries for WIFI & to get time from NTP Server
#include <WiFi.h>
// #include <NTPClient.h>
// #include <WiFiUdp.h>
#include <wificfg.h>
#include "RTClib.h"

// include TCP Client library
#include <TCPClient.h>

// include MQTT Client library
#include <MQTTClient.h>

// Library fo HX711 Access
#include <HX711.h>
#include "HX711Scale.h"

#include "beeiot.h" // provides all GPIO PIN configurations of all sensor Ports !

//************************************
// Global data object declarations
//************************************

// Save reading number on RTC memory
// RTC_DATA_ATTR int loopID = 0;           // generic loop counter

// Define deep sleep options
uint64_t uS_TO_S_FACTOR = 1000000;  // Conversion factor for micro seconds to seconds
// Sleep for 10 minutes = 600 seconds
uint64_t TIME_TO_SLEEP = 600;

// Central Database of all measured values and runtime parameters
dataset		bhdb;
uint16_t	lflags;      // BeeIoT log flag field
Preferences preferences;   // we must generate this object of the preference library

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
extern int islora;              // =0 LoRa client is active

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
int LoRa_interval      = 2000;     // interval between sends

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
// lflags = LOGBH + LOGOW + LOGHX + LOGLAN + LOGEPD + LOGSD + LOGADS + LOGSPI + LOGLORA;
lflags = LOGBH + LOGLORA + LOGHX;

  // put your setup code here, to run once:
  pinMode(LED_RED,   OUTPUT); 
  //  pinMode(LED_GREEN, OUTPUT); // pin resused for LORA Bee Board DIO2
  digitalWrite(LED_RED, LOW);     // signal Setup Phase
//  digitalWrite(LED_GREEN, HIGH); // signal no function active


  while(!Serial);                 // wait to connect to computer
  Serial.begin(115200);           // enable Ser. Monitor Baud rate
  delay(3000);                    // wait for console opening

  Serial.println();
  Serial.println(">*******************************<");
  Serial.println("> BeeIoT - BeeHive Weight Scale <");
  Serial.println(">       by R.Esser 10/2019      <");
  Serial.println(">*******************************<");
  if(lflags > 0)
    Serial.printf ("LogLevel: %i\n", lflags);
  
  BHLOG(LOGBH)Serial.println("Main: Start Sensor Setup ...");

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
    // enter here exit code, if needed
    // isrtc should be -1 here;
    // hopefully NTP can help out later on
  }else{
    BHLOG(LOGLAN) rtc_test();
  }
  
//***************************************************************
  BHLOG(LOGBH) Serial.println("  Setup: SPI Devices ...");
  if (setup_spi_VSPI() != 0){ 
    BHLOG(LOGBH) Serial.println("  Setup: SPI setup failed");
    // enter here exit code, if needed
  }

  // now we have everything for ePaper Welcome Screen
  delay(1000);  // lets enjoy Welcome screen a while

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
    // enter here exit code, if needed
  }

//***************************************************************
  BHLOG(LOGBH) Serial.println("  Setup: LoRa SPI device & Base layer");
  if (setup_LoRa() != 0){
    BHLOG(LOGBH) Serial.println("  Setup: LoRa Base layer setup failed");
    // enter here exit code, if needed
  }

//***************************************************************
  BHLOG(LOGBH)Serial.println("  Setup: ePaper + show start frame ");
  // isepd=-1; // Disable EPD for test purpose only

  if (setup_epd() != 0){
    BHLOG(LOGBH)Serial.println("  Setup: ePaper Test failed");
    // enter here exit code, if needed
  }

//***************************************************************
  BHLOG(LOGBH) Serial.println("  Setup: OneWire Bus setup");
  if (setup_owbus() == 0){
    BHLOG(LOGBH) Serial.println("  Setup: No OneWire devices found");
    // enter here exit code, if needed
  }
  GetOWsensor(0); // read temperature the first time

//***************************************************************
  BHLOG(LOGBH) Serial.println("  Setup: Setup done");
  Serial.println(" ");
} // end of BeeIoT setup()



//*******************************************************************
// BeeIoT Main Routine: Loop()
// 
//*******************************************************************
void loop() {
  // put your main code here, to run repeatedly:
  digitalWrite(LED_RED, HIGH);
  BHLOG(LOGBH) Serial.println(">****************************************<");
  BHLOG(LOGBH) Serial.println("> Main: Start BeeIoT Weight Scale"); 
  BHLOG(LOGBH) Serial.printf ("> Loop: %i  (Laps: %i)\n", bhdb.loopid + (bhdb.laps*datasetsize), bhdb.laps);
  BHLOG(LOGBH) Serial.println(">****************************************<");

  bhdb.dlog[bhdb.loopid].index = bhdb.loopid + (bhdb.laps*datasetsize);


//***************************************************************
// Check for Web Config page update
#ifdef WIFI_CONFIG
//  digitalWrite(LED_GREEN, LOW);
  if(iswifi == 0){
//      BHLOG(LOGLAN) Serial.println("    Check for new WebPage Client request...");
//      CheckWebPage();
  }
//  digitalWrite(LED_GREEN, HIGH);
#endif // WIFI_CONFIG


//***************************************************************
// get current time
//  digitalWrite(LED_GREEN, LOW);
  if(isntp==0 || isrtc==0){ // do we have a valid time source
    getTimeStamp();         // get curr. time to bhdb
    // store curr. timstamp for next data row
    bhdb.dlog[bhdb.loopid].timeStamp = bhdb.timeStamp.substring(0, bhdb.timeStamp.length()-3);
  }else{
    bhdb.timeStamp = "yyyy-mm-dd hh:mm:ss";
    bhdb.dlog[bhdb.loopid].timeStamp = "00:00";
  }
//  digitalWrite(LED_GREEN, HIGH);

//***************************************************************
#ifdef HX711_CONFIG
  float weight;
  // HX711 WakeUp Device
//  digitalWrite(LED_GREEN, LOW);
  scale.power_up();
 
  // Acquire raw reading
  weight = HX711_read(0);
  BHLOG(LOGHX) Serial.printf("  HX711: Weight(raw) : %d", (u_int) weight);
  
  // Acquire unit reading
  weight = HX711_read(1);
  BHLOG(LOGHX) Serial.printf(" - Weight(unit): %.3f kg\n", weight);
  bhdb.dlog[bhdb.loopid].HiveWeight = weight;
  scale.power_down();
//  digitalWrite(LED_GREEN, HIGH);
#endif // HX711_CONFIG

//***************************************************************
#ifdef ONEWIRE_CONFIG
  setup_owbus();
  GetOWsensor(bhdb.loopid);   // Get all temp values directly into bhdb
  while(bhdb.dlog[bhdb.loopid].TempHive > 70){       // if we have just started lets do it again to get right values
    GetOWsensor(bhdb.loopid);   // Get all temp values directly into bhdb
    delay(500);
  }
//  digitalWrite(LED_GREEN, HIGH);
#endif // ONEWIRE_CONFIG

//***************************************************************
#ifdef ADS_CONFIG
 int16_t addata = 0;
  float x;
  //  digitalWrite(LED_GREEN, LOW);
  // read out all ADS channels 0..3
  BHLOG(LOGADS) Serial.print("  MAIN: ADSPort(0-3): ");
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
//  digitalWrite(LED_GREEN, HIGH);
#endif // ADS_CONFIG

//***************************************************************
#ifdef SD_CONFIG
  // scan SDCard if plugged in and report parameters to serial port
//  digitalWrite(LED_GREEN, LOW);
  if(issdcard == 0){
    // spi_scan();
    BHLOG(LOGSD) Serial.println("  MAIN: update of sensor data log");
    SDlogdata();      // save all collected sensor data
    //  showPartialUpdate(weight);
  }else{
    BHLOG(LOGSD) Serial.println("  MAIN: No SDCard, no Logfile");
  }
//  digitalWrite(LED_GREEN, HIGH);
#endif // SD_CONFIG

//***************************************************************
#ifdef EPD_CONFIG
//    digitalWrite(LED_GREEN, LOW);
    BHLOG(LOGEPD) Serial.println("  MAIN: Show Sensor Data on EPD");
    showdata(bhdb.loopid);
//    digitalWrite(LED_GREEN, HIGH);
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
  BHLOG(LOGBH) Serial.printf("  MAIN: Enter Sleep/Wait Mode for %i sec.\n", LOOPTIME);
  // Start deep sleep
  //  esp_sleep_enable_timer_wakeup(SLEEPTIME);    // Deep Sleep Zeit einstellen
  //  esp_deep_sleep_start();                       // Starte Deep Sleep
  // or
  mydelay(LOOPTIME*1000);   // wait with blinking green LED

  BHLOG(LOGBH) Serial.println();
} // end of loop()


//*******************************************************************
// SDlogdata()
// Append current sensor data set to SD card -> logdata file
//*******************************************************************
// Write the sensor readings on the SD card
void SDlogdata(void) {
  String dataMessage; // Global data objects

  if(issdcard !=0){
    BHLOG(LOGSD) Serial.println("  MAIN: No SDCard, no local Logfile...");
    // return;       // do nothing: no sdcard detectd
    // but may be via LoRa ...
  }else{
    dataMessage = String((bhdb.laps*datasetsize) + bhdb.loopid) + "," + 
                String(bhdb.dayStamp) + "," + 
                String(bhdb.timeStamp) + "," + 
                String(bhdb.dlog[bhdb.loopid].HiveWeight) + "," +
                String(bhdb.dlog[bhdb.loopid].TempExtern) + "," +
                String(bhdb.dlog[bhdb.loopid].TempIntern) + "," +
                String(bhdb.dlog[bhdb.loopid].TempHive)   + "," +
                String(bhdb.dlog[bhdb.loopid].TempRTC)    + "," +
                String((float)bhdb.dlog[bhdb.loopid].ESP3V/1000)      + "," +
                String((float)bhdb.dlog[bhdb.loopid].Board5V/1000)    + "," +
                String((float)bhdb.dlog[bhdb.loopid].BattCharge/1000) + "," +
                String((float)bhdb.dlog[bhdb.loopid].BattLoad/1000)   + "," +
                String(bhdb.dlog[bhdb.loopid].BattLevel)  + "\r\n";
    Serial.print("  MAIN: ");
    Serial.print(dataMessage);
    appendFile(SD, SDLOGPATH, dataMessage.c_str());
  }

// do the same via LoRa media
 if(islora==0){  // do we have an active connection (are we joined ?)
     if (millis() - lastSendTime > LoRa_interval) {
     sendMessage(dataMessage);
      lastSendTime = millis();  // timestamp the message
      LoRa_interval = 2000;     // spin up window to 2-3 seconds

      LoRa.receive();           // go back into receive mode
    }
  }else{
      BHLOG(LOGLORA) Serial.println("  MAIN: No LoRa, no Logfile sent...");
  }
  return;
}

void mydelay(int32_t tval){
  int fblink = tval / 1000;   // get # of seconds == blink frequence
  int i;
  for (i=0; i < fblink/2; i++){
    digitalWrite(LED_RED, LOW);
      if(iswifi == 0){
        CheckWebPage();
      }
    delay(1000);  // wait 1 second
    digitalWrite(LED_RED, HIGH);
      if(iswifi == 0){
        CheckWebPage();
      }
    delay(1000);  // wait 1 second
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
  bhdb.formattedDate= "";
  bhdb.dayStamp     = "";
  bhdb.timeStamp    = "";
  bhdb.ipaddr       = "";
  // bhdb.BoardID      = 0;  already defined
  for(i=0; i<datasetsize;i++){
    bhdb.dlog[i].HiveWeight  =0;
    bhdb.dlog[i].TempExtern  =0;
    bhdb.dlog[i].TempIntern  =0;
    bhdb.dlog[i].TempHive    =0;
    bhdb.dlog[i].TempRTC     =0;
    bhdb.dlog[i].timeStamp   ="";
    bhdb.dlog[i].index       =0;
    bhdb.dlog[i].ESP3V       =0;
    bhdb.dlog[i].Board5V     =0;
    bhdb.dlog[i].BattLevel   =0;
    bhdb.dlog[i].BattCharge  =0;
    bhdb.dlog[i].BattLoad    =0;
    strcpy(& bhdb.dlog[i].comment[0], "OK");
  }
  
  // see https://github.com/espressif/arduino-esp32/blob/master/libraries/Preferences/examples/StartCounter/StartCounter.ino 
  preferences.begin("MyJourney", false);

  // takeout 4 Strings out of the Non-volatile storage
  String strSSID      = preferences.getString("SSID", "");
  String strPassword  = preferences.getString("Password", "");
  String strDeviceID  = preferences.getString("DeviceID", "");
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
  BHLOG(LOGLAN) Serial.printf("    Webserver: ProcessConfigValues(%i)\n", countValues);
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
    BHLOG(LOGLAN) Serial.printf("    Webserver: Processing command: %s\n", ConfigValue[0].c_str());
  }else{
    ConfigStatus[0] = -1;   // Value is not valid
    BHLOG(LOGLAN) Serial.printf("    Webserver: Wrong command: %s\n", ConfigValue[0].c_str());
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
      BHLOG(LOGLAN) Serial.printf("    CheckWebPage: GETParameter[%i]=", i);
      BHLOG(LOGLAN) Serial.println(GETParameter);
      if (GETParameter.length() > 1){      // request contains some GET parameter
          
          // decode the GET parameter and set ConfigValues
          int countValues = DecodeGETParameterAndSetConfigValues(GETParameter);
          BHLOG(LOGLAN) Serial.printf("    CheckWebPage: Interpreting <%i> parameters for %s\n", countValues, WebRequestHostAddress.c_str());
          
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
                BHLOG(LOGLAN) Serial.printf("    WebServer: Reconnect SSID:%s - PWD:%s\n", txtSSID, txtPassword);

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