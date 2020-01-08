//*******************************************************************
// wifi.cpp - WiFi and Webservice support routines 
// from Project: https://github.com/mchresse/BeeIoT
//
// Description:
// Contains main setup() and loop() routines for Wifi connections
//
//----------------------------------------------------------
// Copyright (c) 2019-present, Randolph Esser
// All rights reserved.
// This file is distributed under the BSD-3-Clause License
// See LICENSE file in the project root for full license information:
// https://github.com/mchresse/BeeIoT/license
// For used 3rd party open source see also Readme_OpenSource.txt
//*******************************************************************
//
// This code provides incorporated code from 
// - The "espressif/arduino-esp32/preferences" library, which is 
//   distributed under the Apache License, Version 2.0
// - MQTT Library distributed under the MIT-License: 
//   https://opensource.org/licenses/mit-license.php
//   
//*******************************************************************

//*******************************************************************
// Wifi management Local Libraries
//*******************************************************************
// For ESP32-DevKitC PIN Configuration look at BeeIoT.h

#include <Arduino.h>
#include <stdio.h>
#include <esp_log.h>
#include "sdkconfig.h"
#include <Preferences.h>   // this library is used to get access to Non-volatile storage (NVS) of ESP32
// see https://github.com/espressif/arduino-esp32/blob/master/libraries/Preferences/examples/StartCounter/StartCounter.ino

// Libraries used to get Wifi access and time from NTP Server
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>

#include <NTPClient.h>

#include "FS.h"
#include "SD.h"

#include "wificfg.h"
// include MQTT Client library
#include <MQTTClient.h>

#include "beeiot.h"     // provides all GPIO PIN configurations of all sensor Ports !

extern dataset bhdb;    // central BeeIo DB
extern uint16_t	lflags;      // BeeIoT log flag field

extern Preferences preferences;  

// For WiFI & WebServer Instances
int iswifi =-1;               // WIFI flag; =0 connected
String WebRequestHostAddress; // global variable used to store Server IP-Address of HTTP-Request
byte RouterNetworkDeviceState;


WiFiClient myclient;
WiFiServer server(80);          // Instantiate Webserber on port 80


// in main.cpp
extern String ConfigName[8];     // name of the configuration value
extern String ConfigValue[8];    // the value itself (String)
extern int    ConfigStatus[8];   // status of the value    0 = not set    1 = valid   -1 = not valid
extern int    ConfigType[8];     // type of the value    0 = not set    1 = String (textbox)   2 = Byte (slider)


// Code from: MQTT Library distributed under the MIT-License: 
//   https://opensource.org/licenses/mit-license.php

// construct the object attTCPClient of class TCPClient
extern TCPClient attTCPClient;
// construct the object attMQTTClient of class MQTTClient
extern MQTTClient attMQTTClient;


//*******************************************************************
// Wifi Setup Routine
//*******************************************************************
int setup_wifi() { // WIFI Constructor
  iswifi = -1;

#ifdef WIFI_CONFIG
  BHLOG(LOGLAN) Serial.println("  WiFi: Init port in station mode");

  WiFi.mode(WIFI_STA);        // set WIFI chip to station (client) mode
  BHLOG(LOGLAN) wifi_scan();  // show me all networks in space

  // Connect to the WiFi network (see function below loop)
  iswifi = wifi_connect(SSID_HOME, PWD_HOME, HOSTNAME);

  if(iswifi==0){
    Webserver_Start();          // Start Webserver service using new Wifi LAN
  }
#endif // WIFI_CONFIG

  return iswifi;   // Wifi device port is initialized
} // end of setup_wifi()


//*******************************************************************
// BeeIoT Wifi-Scan routine
// Enables WIFI device and Scan all available networks
// finally list them up by ser. line
//*******************************************************************
void wifi_scan(){
#ifdef WIFI_CONFIG
   Serial.print("  Wifi: Scan started");
    // WiFi.scanNetworks will return the number of networks found
    int n = WiFi.scanNetworks();
    Serial.println("...done");
    if (n <= 0) {
        Serial.println("  Wifi: no networks found");
    } else {
        Serial.print("  Wifi: networks found: ");
        Serial.println(n);
        for (int i = 0; i < n; ++i) {
            // Print SSID and RSSI for each network found
            Serial.print("        ");
            Serial.print(i + 1);
            Serial.print(": ");
            Serial.print(WiFi.SSID(i));
            Serial.print(" (");
            Serial.print(WiFi.RSSI(i));
            Serial.print(")");
            Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN)?" ":"*");
            delay(10);
        }
    }
#endif // WIFI_CONFIG
} // end of wifi_scan()

//*******************************************************************
// BeeIoT Wifi-Connect routine
// Enables WIFI device connection to network
// finally enable DNS service for given hostname
//*******************************************************************
int wifi_connect(const char * ssid, const char * pwd, const char* hostname){
int i;

#ifdef WIFI_CONFIG
  BHLOG(LOGLAN) Serial.print("  WIFI: Connect");
  WiFi.begin(ssid, pwd); // request IP address from WLAN

  for(i=0; i<WIFI_RETRIES; i++){
    if(WiFi.status() != WL_CONNECTED) {
      delay(1000);
      BHLOG(LOGLAN) Serial.print(".");
      if(i==2){
        // Probably a disconnect helps after 2. retry (but only once).
        wifi_disconnect();
      }
    }else{
      // Got IPv4 string assumed IP is in v4 format !!! v6 not supported her => cut off
      strncpy(bhdb.ipaddr, WiFi.localIP().toString().c_str(),LENIPADDR); 
      BHLOG(LOGLAN) Serial.printf("ed with %s", ssid);
      BHLOG(LOGLAN) Serial.printf(" - IP: %s\n", bhdb.ipaddr);
      if (MDNS.begin("BeeIoT")) {
        BHLOG(LOGLAN) Serial.println("  WIFI: MDNS-Responder gestartet.");
      }
      return 0; // connected
    }
  }
  
BHLOG(LOGLAN) Serial.println("  WiFi: Connection failed");
#endif // WIFI_CONFIG

return -1;  // not connected
} // end of wifi_connect()

//*******************************************************************
// BeeIoT Wifi-Disconnect routine
// Disconnect from router network and return 1 (success) or -1 (no success)
//*******************************************************************
int wifi_disconnect(){
  int success = -1;
  int WiFiConnectTimeOut = 0;
  
  WiFi.disconnect();
  while ((WiFi.status() == WL_CONNECTED) && (WiFiConnectTimeOut < WIFI_RETRIES))  {
    // try it more than once
    delay(1000);
    WiFiConnectTimeOut++;
  }

  // not connected
  if (WiFi.status() != WL_CONNECTED) {
    success = 1;
  }
  
  BHLOG(LOGLAN) Serial.println("WiFi: Disconnected.");

  return success;
} // end of WIFI_DISCONNECT


//*******************************************************************
// BeeIoT Wifi_AccessPointStart()
// Initialize Soft Access Point with ESP32
// ESP32 establishes its own WiFi network, one can choose the SSID
//*******************************************************************
int WiFi_AccessPointStart(char* AccessPointNetworkSSID){ 
  WiFi.softAP(AccessPointNetworkSSID);

  // printout the ESP32 IP-Address in own network, per default it is "192.168.4.1".
  BHLOG(LOGLAN)   Serial.println(WiFi.softAPIP());

  return 1;
}


//*******************************************************************
// BeeIoT WiFi_SetBothModesNetworkStationAndAccessPoint()
// Put ESP32 in both modes in parallel: Soft Access Point and station in router network (LAN)
//*******************************************************************
void WiFi_SetBothModesNetworkStationAndAccessPoint(){
  WiFi.mode(WIFI_AP_STA);
}


//*******************************************************************
// BeeIoT WiFi_GetOwnIPAddressInRouterNetwork()
// Get IP-Address of ESP32 in Router network (LAN), in String-format
//*******************************************************************
String WiFi_GetOwnIPAddressInRouterNetwork()
{
  return WiFi.localIP().toString();
}

// +++++++++++++++++++ Start of Webserver Library +++++++++++++++++++
// as ESP32 Arduino Version
void Webserver_Start(){
  server.begin();     // Start TCP/IP-Server on ESP32
}

//*******************************************************************************
//  Call this function regularly to look for client requests
//  template see https://github.com/espressif/arduino-esp32/blob/master/libraries/WiFi/examples/SimpleWiFiServer/SimpleWiFiServer.ino
//  returns empty string if no request from any client
//  returns GET Parameter: everything after the "/?" if ADDRESS/?xxxx was entered by the user in the webbrowser
//  returns "-" if ADDRESS but no GET Parameter was entered by the user in the webbrowser
//  remark: client connection stays open after return
//*******************************************************************************
String Webserver_GetRequestGETParameter(){
  String GETParameter = "";
  
  myclient = server.available();   // listen for incoming clients
  //Serial.print(".");
  
  if (myclient) {                            // if you get a client,
    BHLOG(LOGLAN) Serial.println("    WebGet: New WebClient detected");           // print a message out the serial port
    String currentLine = "";                 // make a String to hold incoming data from the client
    
    while (myclient.connected()) {           // loop while the client's connected
      if (myclient.available()) {            // if there are bytes to read from the client,        
        char c = myclient.read();            // read a byte, then
        BHLOG(LOGLAN) Serial.write(c);       // print it out the serial monitor

        if (c == '\n') {                     // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request
          if (currentLine.length() == 0) {            
            if (GETParameter == "") {GETParameter = "-";};    // if no "GET /?" was found so far in the request bytes, return "-"
            // break out of the while loop:
            break;        
          } else {    // if you got a newline, then clear currentLine:
            currentLine = "";
          }          
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }

        if (c=='\r' && currentLine.startsWith("GET /?")) {
          // we see a "GET /?" in the HTTP data of the client request
          // user entered ADDRESS/?xxxx in webbrowser, xxxx = GET Parameter
          // extract everything behind the ? and before a space:
          GETParameter = currentLine.substring(currentLine.indexOf('?') + 1, currentLine.indexOf(' ', 6));
        }

        if (c=='\r' && currentLine.startsWith("Host:")) {
          // we see a "Host:" in the HTTP data of the client request
          // user entered ADDRESS or ADDRESS/... in webbrowser, ADDRESS = Server IP-Address of HTTP-Request
          int IndexOfBlank = currentLine.indexOf(' ');

          // extract everything behind the space character and store Server IP-Address of HTTP-Request
          WebRequestHostAddress = currentLine.substring(IndexOfBlank + 1, currentLine.length());
        }        
      }
    }
  }

  return GETParameter;
} // end of Webserver_GetRequestGETParameter()



//*******************************************************************************
// Send HTML page to client, as HTTP response
// client connection must be open (call Webserver_GetRequestGETParameter() first)
//*******************************************************************************
void Webserver_SendHTMLPage(String HTMLPage){
  String httpResponse = "";
  BHLOG(LOGLAN) Serial.println("  WebServer: SendHTMLPage()");

  // begin with HTTP response header
   httpResponse += "HTTP/1.1 200 OK\r\n";
   httpResponse += "Content-type:text/html\r\n\r\n";

  // then the HTML page
   httpResponse += HTMLPage;

  // The HTTP response ends with a blank line:
   httpResponse += "\r\n";
    
  // send it out to TCP/IP client = webbrowser 
   myclient.println(httpResponse);

  // close the connection
  myclient.stop();
  BHLOG(LOGLAN) Serial.println("  WebServer: Client Disconnected.");   
};


//*************************************************************************
// Build a HTML page with a form which shows textboxes to enter the values
// returns the HTML code of the page
//*************************************************************************
String EncodeFormHTMLFromValues(String TitleOfForm, int CountOfConfigValues){
   // Head of the HTML page
   String HTMLPage = "<!DOCTYPE html><html><body><h2>" + TitleOfForm + "</h2><form><table>";
   BHLOG(LOGLAN) Serial.println("  WebServer: EncodeFormHTMLFromValues()");
   // for each configuration value
   for (int c = 0; c < CountOfConfigValues; c++){
      // set background color by the status of the configuration value
      String StyleHTML = "";
      if (ConfigStatus[c] == 0)  { StyleHTML = " Style =\"background-color: #FFE4B5;\" " ;};  // yellow
      if (ConfigStatus[c] == 1)  { StyleHTML = " Style =\"background-color: #98FB98;\" " ;};  // green
      if (ConfigStatus[c] == -1) { StyleHTML = " Style =\"background-color: #FA8072;\" " ;};  // red

      String TableRowHTML = "";
  
      if (ConfigType[c] == 1){    //  string value, entered in text box
        // build the HTML code for a table row with configuration value name and the value itself inside a textbox   
        TableRowHTML = "<tr><th>" + ConfigName[c] + "</th><th><input name=\"" + ConfigName[c] + "\" value=\"" + ConfigValue[c] + "\" " + StyleHTML + " /></th></tr>";
      }
  
      if (ConfigType[c] == 2){
        // build the HTML code for a table row with configuration value name and the byte value represented by a slider 
        TableRowHTML = "<tr><th>" + ConfigName[c] + "</th><th><input type=\"range\" min=\"1\"  max=\"255\" name=\"" + ConfigName[c] + "\" value=\"" + ConfigValue[c] + "\" " + StyleHTML + " /></th></tr>";
      }
      
      // add the table row HTML code to the page
      HTMLPage += TableRowHTML;
   }

   // add the submit button
   HTMLPage += "</table><br/><input type=\"submit\" value=\"Submit\" />";

   // footer of the webpage
   HTMLPage += "</form></body></html>";
   
   return HTMLPage;
}


//*************************************************************************
// Decodes a GET parameter (expression after ? in URI (URI = expression entered in address field of webbrowser)), like "Country=Germany&City=Aachen"
// and set the ConfigValues
//*************************************************************************
int DecodeGETParameterAndSetConfigValues(String GETParameter){  
   int posFirstCharToSearch = 1;
   int count = 0;

   BHLOG(LOGLAN) Serial.print("  WebServer: DecodeGETParameters():");

// This code goes wrong: counts chars not '&' -> to be fixed !!!

   // while a "&" is in the expression, after a start position to search
   while (GETParameter.indexOf('&', posFirstCharToSearch) > -1){
     int posOfSeparatorChar = GETParameter.indexOf('&', posFirstCharToSearch);  // position of & after start position
     int posOfValueChar = GETParameter.indexOf('=', posFirstCharToSearch);      // position of = after start position
  
     ConfigValue[count] = GETParameter.substring(posOfValueChar + 1, posOfSeparatorChar);  // extract everything between = and & and enter it in the ConfigValue
      
     posFirstCharToSearch = posOfSeparatorChar + 1;  // shift the start position to search after the &-char 
     count++;
   }

   // no more '&' chars found   
   int posOfValueChar = GETParameter.indexOf('=', posFirstCharToSearch);       // search for =
   ConfigValue[count] = GETParameter.substring(posOfValueChar + 1, GETParameter.length());  // extract everything between = and end of string
   count++;

   BHLOG(LOGLAN) Serial.printf("  %i params\n",count);
   return count;  // number of values found in GET parameter
}


// +++++++++++++++++++ End of Webserver library +++++++++++++++++++++


// Connect to AllThingsTalk MQTT Broker and switch RGB-LEDs
int ConnectToATT(){   
    int NodeState;

    // closes TCP connection
    attTCPClient.Close();
       
    // TCP-connect to AllThingsTalk MQTT Server
    int TCP_Connect_Result = attTCPClient.Connect("api.allthingstalk.io", 1883);
    
    // Success?
    if (TCP_Connect_Result != 1){
      // no = red LED
       NodeState = NODESTATE_NOTCPCONNECTION;  
    } else {
      // take authentication strings for AllThingsTalk out of controller memory
      String strDeviceID = preferences.getString("DeviceID", "");
      String strDeviceToken = preferences.getString("DeviceToken", "");

      // send out MQTT CONNECT request with Device Data, Password is not needed by AllThingsTalk, we can set it to "abc"
      int MQTT_Connect_Result = attMQTTClient.Connect(strDeviceID, "maker:" + strDeviceToken, "abc");
      if (MQTT_Connect_Result == 1){
        // connected
        NodeState = NODESTATE_OK;
        Serial.println("MQTT connected.");        
      } else {
        // not connected
        NodeState = NODESTATE_NOMQTTCONNECTION;  

        Serial.println("MQTT not connected.");
      }
   }

   return NodeState;
}



// Publish a MQTT message with SensorDate (here: 1 Byte, value 10..99) to AllThingsTalk MQTT Broker
// SensorName is the asset name for the AllThingsTalk user device
void SendSensorDateToATT(String SensorName, byte SensorDate){
    // Dummy for message payload in JSON format
    String MString = "{\"value\": 00.0}";
  
    int Slength = MString.length();

    // copy the bytes of the Dummy string in the payload buffer
    for (int j = 0; j < Slength; j++)
    {
        attMQTTClient.PayloadBuffer[j] = MString[j];
    }

    // encode SensorDate Byte (10..99) in ASCII format inside the payload JSON expression
    attMQTTClient.PayloadBuffer[10] = (SensorDate / 10) + 48;
    attMQTTClient.PayloadBuffer[11] = (SensorDate % 10) + 48;

    // Device ID for AllThingsTalk out of controller memory
    String strDeviceID = preferences.getString("DeviceID", "");

    // generate the appropriate Topic for AllThingsTalk MQTT Broker with Device data
    String ATT_SensorTopic = "device/" + strDeviceID + "/asset/" + SensorName + "/state";

    // Publish the message with topic and payload
    attMQTTClient.Publish(ATT_SensorTopic, attMQTTClient.PayloadBuffer, Slength);
}
// end of wifi.cpp