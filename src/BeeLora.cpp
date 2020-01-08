//*******************************************************************
// BeeLoRa.cpp
// from Project: https://github.com/mchresse/BeeIoT
//
// Description:
// LoRa MAC layer - Support routines for BeeIoT-WAN Protocol
//
//----------------------------------------------------------
// Copyright (c) 2019-present, Randolph Esser
// All rights reserved.
// This file is distributed under the BSD-3-Clause License
// See LICENSE file in the project root for full license information:
// https://github.com/mchresse/BeeIoT/license
// For used 3rd party open source see also Readme_OpenSource.txt
//*******************************************************************

//*******************************************************************
// LoRa local Libraries
//*******************************************************************
// For ESP32-DevKitC PIN Configuration look at BeeIoT.h

#include <Arduino.h>
#include <stdio.h>

#include <LoRa.h>       // from LoRa Sandeep Driver 
#include "beelora.h"    // BeeIoT Lora Message definitions

#include "beeiot.h"     // provides all GPIO PIN configurations of all sensor Ports !

//***********************************************
// LoRa MAC Presets
//***********************************************
#define LORA_SF   7           // preset spreading factor
#define LoRaWANSW 0x34	      // LoRa WAN public sync word; def: 0x34
#define SIGNALBW  125E3       // default Signal bandwidth
#define LORA_FREQ 8681E5      // Band: 868 MHz
#define LORA_PWR  14          // Power Level: 14dB

#define CODINGRATE 5
#define LPREAMBLE  8

//***********************************************
// Global Sensor Data Objects
//***********************************************
extern uint16_t	lflags;           // BeeIoT log flag field
int     islora;                   // Semaphor of active LoRa Node (=0: active)

const int csPin     = BEE_CS;     // LoRa radio chip select
const int resetPin  = BEE_RST;    // LoRa radio reset
const int irqPin    = BEE_DIO0;   // change for your board; must be a hardware interrupt pin

byte localAddress = BEEIOT_DEVID; // address/ID of this device
byte destination  = BEEIOT_GWID;  // destination ID to send to

byte msgCount = 0;        // gobal serial counter of outgoing messages 0..255 round robin
beeiotmsg_t MyMsg;        // Lora message on the air (if MyMsg.data != NULL)

byte    BeeIotRXFlag =0;  // Semaphor for received message(s) -> polled by main()
beeiotdata_t MyRXData;    // received message for userland processing

//*********************************************************************
// Setup_LoRa(): init BEE-client object: LoRa
//*********************************************************************
int setup_LoRa(){
    BHLOG(LOGLORA) Serial.printf("  LoRa: Start Lora SPI Init\n");
    islora = -1;

 if (!LoRa.begin(LORA_FREQ)) {          // initialize ratio at LORA_FREQ [MHz] 
                                        // for SX1276 only (Version = 0x12)
    Serial.println("  LoRa: SX1276 not detected. Check your GPIO connections.");
    return(islora);                          // No SX1276 module found -> Stop LoRa protocol
  }

    // Presetting of all LoRa protocol parameters if apart from the class default
    LoRa.sleep();
    // RFOUT_pin could be RF_PACONFIG_PASELECT_PABOOST or RF_PACONFIG_PASELECT_RFO
    //   - RF_PACONFIG_PASELECT_PABOOST -- LoRa single output via PABOOST, maximum output 20dBm
    //   - RF_PACONFIG_PASELECT_RFO     -- LoRa single output via RFO_HF / RFO_LF, maximum output 14dBm
    LoRa.setSpreadingFactor(LORA_SF);     // ranges from 6-12,default 7 see API docs
    LoRa.setSyncWord(LoRaWANSW);          // ranges from 0-0xFF, default 0x34, see API docs
    LoRa.setTxPower(LORA_PWR,PA_OUTPUT_PA_BOOST_PIN); // no boost mode, outputPin = default
    LoRa.setSignalBandwidth(SIGNALBW);    // set Signal Bandwidth     LoRa.setCodingRate4(CODINGRATE);
    LoRa.setPreambleLength(LPREAMBLE);
    LoRa.crc();                           // enable CRC
    LoRa.idle();
  
//  LoRa.dumpRegisters(Serial);         // dump all SX1276 registers in 0xyy format for debugging
  BeeIotRXFlag =0;                      // reset Semaphor for received message(s) -> polled by main()
  msgCount = 0;                         // no package sent by now
  LoRa.onReceive(onReceive);            // set IRQ callback function
  LoRa.receive();                       // start flow control
  BHLOG(LOGLORA) Serial.println("  LoRa: entered receive mode");

  islora=0;                             // -> LORA is active
  BHLOG(LOGLORA) Serial.println("  LoRa: init succeeded.");

  return(islora);
} // enfd of Setup_LoRa()

//****************************************************************************************
// LoraLog()
// Converts UserLog-String into LoRa Mesage fomat
// Controls flow control: Send -> Wait for Ack ... in case send retries.
//****************************************************************************************
int LoRaLog(byte cmd, String outgoing) {
beeiotdata_t MyTXData;    // Lora package buffer for sending
int length;
int rc;

  length = (int) outgoing.length();     // get user-payload length
  if(length-5 > MAX_PAYLOAD_LENGTH)     // exceeds buffer -5 byte user header (see below)
    length=MAX_PAYLOAD_LENGTH;          // limit payload length

  // remember new LoRa paket (for mutual retries)
  // add user header (+5 Bytes)
  MyTXData.destID = destination;
  MyTXData.sendID = localAddress;
  MyTXData.index  = msgCount;
  MyTXData.cmd    = cmd;
  MyTXData.length = length;

  strncpy(MyTXData.status, outgoing.c_str, length);

// if (millis() - lastSendTime > LoRa_interval) {
      rc=0;
      for(int i=0; i<MSGBURSTRETRY || rc==1; i++){
        rc = sendMessage(&MyTXData);
        delay(MSGBURSTWAIT);    // minimum time for ACK to arrive
        if(BeeIotRXFlag >0){
          // we've got a message in MyRxData
        }

        // -> polling for callback Action
        // by parsing RXData package
      }
//      lastSendTime = millis();  // timestamp the message
//      LoRa_interval = 2000;     // spin up window to 2 seconds
//  }

  MyMsg.data = NULL;    // RX done -> release LoRa package buffer
  MyMsg.ack  = 0;
  msgCount++;           // increment global sequ. package/message ID
  return(1);
}

//****************************************************************************************
// SendMessage()
// Converts UserLog-String into LoRa Mesage fomat
// Controls flow control: Send -> Wait for Ack ... in case send retries.
// return:
//    1: data was sent successfully
//****************************************************************************************
int sendMessage(beeiotdata_t * TXData) {
int length;

// start package creation:
  if(LoRa.beginPacket(0) ==0)           // reset FIFO ptr.; in explicit header Mode
    return(0);                          // still transmitting -> have to come back later

  // remember current package for callback function
  MyMsg.idx = msgCount;                 // claim new ID of next message on air
  MyMsg.retries = 0;                    // by now no retries needed
  MyMsg.data = TXData;                  // save link to User payload data
  MyMsg.ack = 0;                        // reset Acknowledge flag for callback()

  // now lets fill up FIFO:
  LoRa.write(TXData->destID);          // add destination address
  LoRa.write(TXData->sendID);          // add sender address
  LoRa.write(TXData->index);           // add message ID = package serial number
  LoRa.write(TXData->cmd);             // store function/command for GW 
  LoRa.write(TXData->length);          // user-payload length
  LoRa.print(TXData->status);          // add user-payload
  
  LoRa.endPacket(0);                   // finish packet and send it in LoRa Mode
  // =0: sync mode: endPacket returns when TX_DONE Flag was achieved
  // =1 async: polling via LoRa.isTransmitting() needed; -> remove LoRa.idle()
  LoRa.idle();                          // lay back:  wait for Ack-callback()

  BHLOG(LOGLORA) Serial.printf("  LoRa[0x%x > 0x%x]: #%i(%d)%s - %i bytes\n", 
        TXData->sendID, TXData->destID, TXData->index, TXData->cmd, TXData->status, TXData->length);
  BHLOG(LOGLORA) hexdump((unsigned char*) TXData, sizeof(*TXData));

return(1);
}


// IRQ callback function -> package received (called by LoRa.handleDio0Rise() )
void onReceive(int packetSize) {
// at that point all IRQ flags are cleared, CRC check done, Header Mode processed and
// FIFO ptr (REG_FIFO_ADDR_PTR) was reset to package start -> use LoRa.read() 

  if (packetSize == 0) return;          // if there's no user packet payload, ignore it.

// from here: assumed explicite header mode: -> REG_RX_NB_BYTES Bytes
  // read expected packet user-header bytes leading user-payload (first 5 bytes)
  byte recipient      = LoRa.read();    // recipient address
  byte sender         = LoRa.read();    // sender address
  byte incomingMsgId  = LoRa.read();    // incoming msg ID/counter
  byte incomingMsgCmd = LoRa.read();    // incoming msg user command/function
  byte incomingLength = LoRa.read();    // incoming user msg length

  String incoming = "";                 // payload of packet

  while (LoRa.available()) {            // can't use readString() in callback, so
    incoming += (char)LoRa.read();      // add bytes one by one for REG_RX_NB_BYTES Bytes
  }
  BHLOG(LOGLORA) Serial.printf("  LoRa: onReceive(): Got a message: Recipient:%i, Sender:%i, MsgID: %i, len:%i\n",
      recipient, sender, incomingMsgId, incomingLength);

  if (incomingLength != incoming.length()) {   // check user payload length for completeness
    Serial.println("  LoRa: error: IN-message length does not match length in pkg-header");
    return;                             // skip rest of function
  }

  // if the recipient isn't this device or broadcast,
  if (recipient != localAddress && recipient != 0xFF) { // 0xFF == for everyone -> broadcast
    Serial.println("  LoRa: This message is not for me.");
    return;                             // skip rest of function
  }


  // ToDo: copy message to somewhere for processing and set message semaphor
  // if message is for this device, or broadcast, print details:
  switch(incomingMsgCmd){
    case CMD_LOGSTATUS: {   // Just ACK of sent sensor data package
          BHLOG(LOGLORA) Serial.println("    Received ACK from: 0x" + String(sender, HEX));
          break;            // do nothing
    }
  }

  BeeIotRXFlag = 1;   // tell user process we've got a new message

  // dump info for debugging:
  BHLOG(LOGLORA) Serial.println("    Received from: 0x" + String(sender, HEX));
  BHLOG(LOGLORA) Serial.println("    Sent     to  : 0x" + String(recipient, HEX));
  BHLOG(LOGLORA) Serial.println("    Message ID   : " + String(incomingMsgId));
  BHLOG(LOGLORA) Serial.println("    Message Cmd  : " + String(incomingMsgCmd));
  BHLOG(LOGLORA) Serial.println("    MessageLength: " + String(incomingLength));
  BHLOG(LOGLORA) Serial.println("    RSSI         : " + String(LoRa.packetRssi()));
  BHLOG(LOGLORA) Serial.println("    SNR          : " + String(LoRa.packetSnr()));
  BHLOG(LOGLORA) Serial.println("    Message      : " + incoming);
  BHLOG(LOGLORA) Serial.println();
}

//******************************************************************************
// print hexdump of array  in the format:
// 0xaaaa:  dddd dddd dddd dddd  dddd dddd dddd dddd  <cccccccc cccccccc>
void hexdump(unsigned char * msg, int len){
	int i, y, count;
	unsigned char c;

  Serial.printf("MSGfield at 0x%X:\n", (unsigned int) msg);
	Serial.printf("Address:  0 1  2 3  4 5  6 7   8 9  A B  C D  E F  lenght=%iByte\n", len);
	count = 0;
	while(count < len){
		// print len\16 lines each of 4 x 4 words
		Serial.printf("  +%4X: ", (uint32_t)count);
		for(y=0; y<16; y++){
			Serial.printf("%02X", (uint8_t) msg[count++]);
			if (count == len)
				break;
			Serial.printf("%02X ", (uint8_t) msg[count++]);
			y++;
			if (count == len)
				break;
			if(y==7)
				Serial.printf(" ");
		}

		if(y<16){	// break condition reached: end of byte field
			// at this point y-1 bytes already printed (0..15)
			// we have to fill up the line with " "
			i=y;				// remember # of already printed bytes-1
			if((y+1)%2 ==1){	// do we have a split word ?
				Serial.printf("   ");	// yes, fill up the gap
				i++;			// one byte less
			}
			for( ; i<15; i++)	// fill up the rest bytes of the line
				Serial.printf("  ");
			if(y<7)				// already reached 2. half ?
				Serial.printf(" ");	// no, fill up gap
			for(i=0; i<((15-y)*2)/4; i++)	// fill up gap between each word
				Serial.printf(" ");

			y++;	// compensate break condition of 'for(; ;y++)'
		}
		// just line end reached -> wrap the line
		// print text representation of each line		
		Serial.printf(" <");
		// start with regular letters
		for (i=0; i<y; i++){
			c = msg[count-y+i];
			if(c < 32 || c >= 127)
				Serial.printf(".");
			else
				Serial.printf("%c", (char)c);
			if(i==7) printf(" ");
		}
		Serial.printf(">\n");	// end of full text field print
	}
  return;	
}