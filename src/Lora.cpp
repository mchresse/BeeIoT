//*******************************************************************
// LoRa Core layer - Support routines
//*******************************************************************
// For ESP32-DevKitC PIN Configuration look at BeeIoT.h

//*******************************************************************
// LoRa local Libraries
//*******************************************************************

#include <Arduino.h>
#include <stdio.h>
#include "beeiot.h"     // provides all GPIO PIN configurations of all sensor Ports !
#include <LoRa.h>

//***********************************************
// Global Sensor Data Objects
//***********************************************
extern uint16_t	lflags;      // BeeIoT log flag field
int     islora;              // Semaphor of active LoRa Node (=0: active)

const int csPin     = BEE_CS;     // LoRa radio chip select
const int resetPin  = BEE_RST;    // LoRa radio reset
const int irqPin    = BEE_DIO0;   // change for your board; must be a hardware interrupt pin

String outgoing;              // outgoing message
byte msgCount = 0;            // count of outgoing messages
byte localAddress = 0x77;     // address/ID of this device
byte destination  = 0x88;     // destination ID to send to

#define LORA_SF   7           // preset spreading factor
#define LoRaWANSW 0xF3	      // LoRa WAN public sync word
#define SIGNALBW  0x125E3     // default Signal bandwidth
#define LORA_FREQ 8681E5
#define LORA_PWR  14

//*********************************************************************
// Setup_LoRa(): init BEE-client object: LoRa
//*********************************************************************
int setup_LoRa(){
    BHLOG(LOGLORA) Serial.printf("    Setup: Start Lora SPI Init\n");
    islora = -1;

 if (!LoRa.begin(LORA_FREQ)) {          // initialize ratio at LORA_FREQ [MHz] for SX1276 only (Version = 0x12)
    Serial.println("    LoRa: SX1276 not detected. Check your GPIO connections.");
    return(islora);                          // No SX1276 module found -> Stop LoRa protocol
  }

// Presetting of all LoRa protocol parameters apart from the class default
  LoRa.setSpreadingFactor(LORA_SF);     // ranges from 6-12,default 7 see API docs
  LoRa.setSyncWord(LoRaWANSW);          // ranges from 0-0xFF, default 0x34, see API docs
  LoRa.setTxPower(LORA_PWR);            // no boost mode, outputPin = default
  LoRa.setSignalBandwidth(SIGNALBW);    // set Signal Bandwidth 
//  LoRa.dumpRegisters(Serial);         // dump all SX1276 registers in 0xyy format for debugging

  LoRa.onReceive(onReceive);            // set callback function
  LoRa.receive();                       // start flow control

  islora=0;                             // -> LORA is active
  BHLOG(LOGLORA) Serial.println("    LoRa: init succeeded.");

    return(islora);
}

void sendMessage(String outgoing) {
  LoRa.beginPacket();                   // start packet
  LoRa.write(destination);              // add destination address
  LoRa.write(localAddress);             // add sender address
  LoRa.write(msgCount);                 // add message ID
  LoRa.write(outgoing.length());        // add payload length
  LoRa.print(outgoing);                 // add payload
  LoRa.endPacket();                     // finish packet and send it
  BHLOG(LOGLORA) Serial.printf("    LoRa[0x%x]: Sent #%i:<%s> to 0x%x - %i bytes\n", localAddress, msgCount, outgoing.c_str(), destination, outgoing.length());
  msgCount++;                           // increment message ID
}

void onReceive(int packetSize) {
  if (packetSize == 0) return;          // if there's no packet, return

  // read packet header bytes:
  int recipient = LoRa.read();          // recipient address
  byte sender = LoRa.read();            // sender address
  byte incomingMsgId = LoRa.read();     // incoming msg ID
  byte incomingLength = LoRa.read();    // incoming msg length

  String incoming = "";                 // payload of packet

  while (LoRa.available()) {            // can't use readString() in callback, so
    incoming += (char)LoRa.read();      // add bytes one by one
  }
  BHLOG(LOGLORA) Serial.printf("    LoRa: onReceive(): Got a message: Recip:%i, Sender:%i, MsgID: %i, len:%i\n",
      recipient, sender, incomingMsgId, incomingLength);

  if (incomingLength != incoming.length()) {   // check length for error
    Serial.println("    LoRa: error: IN-message length does not match length in pkg-header");
    return;                             // skip rest of function
  }

  // if the recipient isn't this device or broadcast,
  if (recipient != localAddress && recipient != 0xFF) {
    Serial.println("This message is not for me.");
    return;                             // skip rest of function
  }

  // if message is for this device, or broadcast, print details:
  BHLOG(LOGLORA) Serial.println("Received from: 0x" + String(sender, HEX));
  BHLOG(LOGLORA) Serial.println("Sent to      : 0x" + String(recipient, HEX));
  BHLOG(LOGLORA) Serial.println("Message ID   : " + String(incomingMsgId));
  BHLOG(LOGLORA) Serial.println("Messagelength: " + String(incomingLength));
  BHLOG(LOGLORA) Serial.println("RSSI         : " + String(LoRa.packetRssi()));
  BHLOG(LOGLORA) Serial.println("SNR          : " + String(LoRa.packetSnr()));
  BHLOG(LOGLORA) Serial.println("Message      : " + incoming);
  BHLOG(LOGLORA) Serial.println();
}