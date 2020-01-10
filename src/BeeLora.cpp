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

byte    BeeIotRXFlag =0;            // Semaphor for received message(s) 0 ... MAXRXPKG-1
byte    RXPkgIsrIdx  =0;            // index on next LoRa Package for ISR callback Write
byte    RXPkgSrvIdx  =0;            // index on next LoRa Package for Service Routine Read/serve
beeiotpkg_t MyRXData[MAXRXPKG];     // received message for userland processing

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
    LoRa.idle();                          // enter LoRa Standby Mode
  
//  LoRa.dumpRegisters(Serial); // dump all SX1276 registers in 0xyy format for debugging
  BeeIotRXFlag =0;              // reset Semaphor for received message(s) -> polled by Sendmessage()
  msgCount = 0;                 // no package sent by now
  RXPkgSrvIdx = 0;              // preset RX Queue Read Pointer
  RXPkgIsrIdx = 0;              // preset RX Queue Write Pointer
  LoRa.onReceive(onReceive);    // assign IRQ callback (called by loRa.handleDio0Rise(pkglen))
  islora=0;                     // Declare:  LORA is active
  BHLOG(LOGLORA) Serial.println("  LoRa: init done");
  
  // Now activate flow control
  LoRa.receive();               // Set OpMode: STDBY -> RXContinuous in expl. Header mode          
  BHLOG(LOGLORA) Serial.println("  LoRa: enter RXCont mode");


  return(islora);
} // enfd of Setup_LoRa()



//****************************************************************************************
// LoraLog()
// Converts UserLog-String into LoRa Message format
// Controls flow control: Send -> Wait for Ack ... in case send retries.
// Return:
//    see return codes from BeeIoTParse()
//****************************************************************************************
int LoRaLog(byte cmd, const char * outgoing, byte outlen, int noack) {
beeiotpkg_t MyTXData;    // Lora package buffer for sending
byte length;
int i;
int rc;

  length = outlen;     // get user-payload length
  if(length > BEEIOT_DLEN){      // exceeds buffer user header (see below)
    BHLOG(LOGLORA) Serial.printf("  LoRaLog: data length exceeded, had to cut 0x%x -> 0x%x\n", length, BEEIOT_DLEN);
    length = BEEIOT_DLEN;        // limit payload length incl. EOL(0D0A)
  }
  // remember new LoRa paket (for mutual retries)
  // add user header (+5 Bytes)
  MyTXData.destID = destination;    // remote target GW
  MyTXData.sendID = localAddress;   // that's me
  MyTXData.index  = msgCount;       // ser. number of sent packages
  MyTXData.cmd    = cmd;            // define type of action/data sent
  MyTXData.length = length;         // length of user payload data incl. any EOL

  strncpy(MyTXData.data, outgoing, length);
  MyTXData.data[length]=0;  // limit byte string with EOL = 0 (0D0A is not needed)
  rc=0;
  i=0;

  while(rc == 0) rc=sendMessage(&MyTXData, 0);   // send it in sync mode
  if(noack){ // bypass RX Queue handling -> no wait for ACK
    // fake: sent package was acknowledged -> we are free for new LoRa actions
    MyMsg.data = NULL;    // RX done -> release LoRa package buffer
    MyMsg.ack  = 0;
    msgCount++;           // increment global sequ. TX package/message ID
    BHLOG(LOGLORA) Serial.printf("  LoRaLog: Msg sent & RX Queue Empty SrvIdx:%i, IsrIdx:%i, NextMsgID:%i, RXFlag:%i\n",
      RXPkgSrvIdx, RXPkgIsrIdx, msgCount, BeeIotRXFlag);
    rc=0;
    return(rc);
  }

  BHLOG(LOGLORA) Serial.printf("  LoRaLog: wait for incoming RX Flag:"); 
  // Lets wait for arriving ACK Msg
  while (!BeeIotRXFlag){;   // do we have at least one BeeIoT Message in the Queue ?
    delay(MSGBURSTWAIT);    // minimum time for ACK to arrive -> polling rate
    BHLOG(LOGLORA) Serial.print("."); 
    if(i++ > MAXRXACKWAIT){
      rc=-99;  // timeout
      BHLOG(LOGLORA) Serial.printf("timeout\n"); 
      // ToDo: initiate a retry loop here
      msgCount++;           // increment global sequ. TX package/message ID
      return(rc);
    }
  }
  BHLOG(LOGLORA) Serial.println("-> received"); 

  do{ // lets check the RX Message QUEUE
    // expect ACK in received package at RX Queue 
    rc=BeeIoTParse(&MyRXData[RXPkgSrvIdx]);    // parse next MyRXData[RXPkgSrvIdx] for action 
    if(rc < 0){
      BHLOG(LOGLORA) Serial.printf("  LoRaLog: parsing of Msg[%i]failed rc=%i\n", MyRXData[RXPkgSrvIdx].index, rc);
      // ToDo: Exit Code here ???
      // RXPkgSrvIdx++;
    }
    BeeIotRXFlag--;   // can release one more RX Queue buffer
    RXPkgSrvIdx++;    // point to next RX Queue buffer

    if(rc == CMD_ACK){
      // sent package was acknowledged -> we are free for new LoRa actions
      MyMsg.data = NULL;    // RX done -> release LoRa package buffer
      MyMsg.ack  = 0;
    }
    if(rc == CMD_RETRY){
      MyMsg.retries++;                        // remember we have sent it again
      if (MyMsg.retries > MSGMAXRETRY){
        BHLOG(LOGLORA) Serial.printf("  LoRaLog: # of retries exceeded for Msg[%i] -> give up\n", MyMsg.idx);
        rc = -1;    // give up, transfer media (LoRa) does not work
      }else{
        // ToDo: do we need a grace time in case of retries to save Duty Time ?
        while(rc == 0) rc=sendMessage(&MyTXData, 0);   // send it in sync mode
        BHLOG(LOGLORA) Serial.printf("  LoRaLog: Retry_wait for RX Flag:"); 
        // Lets wait for arriving ACK Msg
        while (!BeeIotRXFlag){;   // do we have at least one BeeIoT Message in the Queue ?
            delay(MSGBURSTWAIT);    // minimum time for ACK to arrive -> polling rate
            BHLOG(LOGLORA) Serial.print("."); 
            if(i++ > MAXRXACKWAIT){
              BHLOG(LOGLORA) Serial.printf("timeout\n"); 
              rc=-99;  // RX timeout
              break;
            }
        }
      }
    }
  } while (rc == CMD_RETRY || BeeIotRXFlag); // for RETRY or remaining Queue items -> we have to do it again

  msgCount++;           // increment global sequ. TX package/message ID
  BHLOG(LOGLORA) Serial.printf("  LoRaLog: Msg sent & RX Queue Empty SrvIdx:%i, IsrIdx:%i, NextMsgID:%i, RXFlag:%i\n",
      RXPkgSrvIdx, RXPkgIsrIdx, msgCount, BeeIotRXFlag);

  return(rc);
}


//******************************************************************************
// BeeIoTParse() 
// Analyze received BeeIoT Message for action
// Input:
//  Msg   Pointer to next BeeIoT Message at MyRXData[RXPkgSrvIdx] for parsing
// Return: rc
//	0	  Msg Status o.k.; Action processed successfully (e.g. NOP) -> nothing to do
//  CMD_LOGSTATUS:  ignored
//  CMD_SDLOG:      Requested SDLOG data
//  CMD_ACK:        ACK to pending TX msg received
//  CMD_RETRY:      ToDo: Retry Request for pending TX
//  -1              Retry Request /wo pending TX (mismatching msgidx)
//  -2              n.a.
//  -3              n.a.
//  -5              unknown CMD code -> ignored
//******************************************************************************
int BeeIoTParse(beeiotpkg_t * msg){
int rc;

// We have a new valid BeeIoT Package (header and data length was checked already): 
// check the "msg->cmd" for next action from GW
	switch (msg->cmd){
	case CMD_NOP:
		// intended to do nothing -> test message for communication check
		// for test purpose: dump payload in hex format
    BHLOG(LOGLORA) Serial.printf("  BeeIoTParse[%i]: cmd= NOP -> do nothing\n", msg->index); 
		hexdump((unsigned char*) msg, msg->length);		
		rc= 0;	
		break;

	case CMD_LOGSTATUS: // BeeIoT node sensor data set received
    BHLOG(LOGLORA) Serial.printf("  BeeIoTParse[%i]: cmd= LOGSTATUS -> should be ACK => ignored\n", msg->index); 
    // intentionally do nothing on node side
    rc=CMD_LOGSTATUS;
		break;

	case CMD_SDLOG:
    BHLOG(LOGLORA) Serial.printf("  BeeIoTParse[%i]: SDLOG Save command: not supported yet\n", msg->index); 
		// for test purpose: dump payload in hex format
		hexdump((unsigned char*) msg, msg->length);		
			// ToDo: read out SD card and send it package wise to GW
		rc= CMD_SDLOG; // not supported yet
		break;

	case CMD_RETRY:
    BHLOG(LOGLORA) Serial.printf("  BeeIoTParse[%i]: RETRY-Request: send same message again\n", msg->index); 
    if(MyMsg.data->index == msg->index){    // do we talk about the same Node-TX message we are waiting for ?
      rc=CMD_RETRY;                         // have to be done by calling level
    }else{
    rc = -1;
    }
		break;

	case CMD_ACK:
    BHLOG(LOGLORA) Serial.printf("  BeeIoTParse[%i]: ACK received -> Data transfer done.\n", msg->index); 
    if(MyMsg.data->index == msg->index){    // save link to User payload data
      MyMsg.ack = 0;                        // reset Acknowledge flag for callback()
    }
    rc = CMD_ACK;
		break;

	default:
    BHLOG(LOGLORA) Serial.printf("  BeeIoTParse[%i]: unknown CMD(%d)\n", msg->index, msg->cmd); 
		// for test purpose: dump paload in hex format
		hexdump((unsigned char*) msg, msg->length);		
			// ToDo: lets acknowledge received package to sender
			//			txlora((byte*)message, receivedbytes);
		rc= -5;
		break;
	}
	
	return(rc);
}


//****************************************************************************************
// SendMessage()
// Converts UserLog-String into LoRa Mesage fomat
// Controls flow control: Send -> Wait for Ack ... in case send retries.
// Input:
//  TXData  User payload structure to be sent
//  Sync    =0 sync mode: wait for TX Done flag
//          =1 async mode: return immediately after TX -> no wait for TXDone
// return:
//    1: data was sent successfully
//****************************************************************************************
int sendMessage(beeiotpkg_t * TXData, int sync) {

// start package creation:
  if(LoRa.beginPacket(0) ==0)           // reset FIFO ptr.; in explicit header Mode
    return(0);                          // still transmitting -> have to come back later

  // remember current package for callback function
  MyMsg.idx = msgCount;                 // claim new ID of next message on air
  MyMsg.retries = 0;                    // by now no resend done
  MyMsg.data = TXData;                  // save link to User payload data
  MyMsg.ack = 0;                        // reset Acknowledge flag for callback()

  // now lets fill up FIFO:
  LoRa.write(TXData->destID);          // add destination address
  LoRa.write(TXData->sendID);          // add sender address
  LoRa.write(TXData->index);           // add message ID = package serial number
  LoRa.write(TXData->cmd);             // store function/command for GW 
  LoRa.write(TXData->length);          // user-payload length
  LoRa.print(TXData->data);            // add user-payload
  
  // Finish packet and send it in LoRa Mode
  // =0: sync mode: endPacket returns when TX_DONE Flag was achieved
  // =1 async: polling via LoRa.isTransmitting() needed; -> remove LoRa.idle()
  LoRa.endPacket(sync);

  BHLOG(LOGLORA) Serial.printf("  LoRaSend[0x%x > 0x%x]: #%i(%d)%s - %i bytes\n", 
        TXData->sendID, TXData->destID, TXData->index, TXData->cmd, TXData->data, TXData->length);
  BHLOG(LOGLORA) hexdump((unsigned char*) TXData, TXData->length);
  
  // Now activate RX Contiguous flow control
  LoRa.receive();                     // Set OpMode: STDBY -> RXContinuous in expl. Header mode          
  BHLOG(LOGLORA) Serial.println("  LoRaSend: enter RXCont mode");

  return(1);
}


//******************************************************************************
// IRQ User callback function 
// => package received (called by LoRa.handleDio0Rise() )
// Input:
//  packetSize  length of recognized RX package user payload in FIFO
//******************************************************************************
void onReceive(int packetSize) {
// at that point all IRQ flags are cleared, CRC check done, Header Mode processed and
// FIFO ptr (REG_FIFO_ADDR_PTR) was reset to package start
// simply use LoRa.read() to read out the FIFO
beeiotpkg_t * msg;
byte index;
byte * ptr;
byte len;

  if(BeeIotRXFlag == MAXRXPKG){ // Check RX Semaphor: RX-Queue full ?
    // same situation as: RXPkgIsrIdx+1 == RXPkgSrvIdx (but hard to check with ring buffer)
    Serial.printf("onReceive: InQueue full (%i items)-> ignored new IRQ with Len 0x%x\n", (byte) BeeIotRXFlag, (byte) packetSize);
    return;     // User service must work harder
  }

  // is it a package size conform to BeeIoT WAN definitions ?
  if (packetSize < BEEIOT_HDRLEN || packetSize > MAX_PAYLOAD_LENGTH) {
    Serial.printf("onReceive: New RXPkg exceed BeeIoT pkg size: len=0x%x\n", packetSize);
    return;          // no payload of interest for us, ignore it.
  }

  index = RXPkgIsrIdx;         // claim new RX package buffer

  // from here: assumed explicite header mode: -> Size = REG_RX_NB_BYTES Bytes
  // read expected packet user-header bytes leading BeeIoT-WAN header
  ptr = (byte*) & MyRXData[index];
  while (LoRa.available()) {      // as long as FiFo Data bytes left
    ptr += (byte)LoRa.read();     // read bytes one by one for REG_RX_NB_BYTES Bytes
  }
  msg = & MyRXData[index];        // Assume: BeeIoT-WAN Message received

 	len = msg->length;	            // 1. assumption:  length value is correct
	
	// some debug output: assumed all is o.k.
	BHLOG(LOGLORA) Serial.printf("onReceive: RX(0x%02X > 0x%02X): ", (unsigned char)msg->sendID, (unsigned char)msg->destID);
	BHLOG(LOGLORA) Serial.printf("[%2i](cmd:%02d) ", (unsigned char) msg->index, (unsigned char) msg->cmd);
  BHLOG(LOGLORA){
    if(msg->cmd == CMD_LOGSTATUS){
      msg->data[len-2]=0;	// than limit string -> set new end marker (and cut off EOL:0D0A)
      Serial.printf("%s ", msg->data); // to be checked if it is a string
      Serial.printf("%i Bytes\n", len);
    }else{
      hexdump((unsigned char*) msg, packetSize);
    }
  } // end of debug print

// now check real data:
// start the gate keeper of valid BeeIoT Package data:
	if(msg->sendID != localAddress){
		Serial.printf("BeeIoTSTatus: Unknown NodeID detected (0x%02X) -> package rejected\n", (unsigned char)msg->sendID);		
    hexdump((unsigned char*) msg, packetSize); 		// for test purpose: dump paload in hex format
		Serial.printf("\n");
		return;
	}
	if(msg->destID != destination){
		Serial.printf("BeeIoTSTatus: Wrong Target GWID detected (0x%02X) -> package rejected\n", (unsigned char)msg->sendID);		
    hexdump((unsigned char*) msg, packetSize); 		// for test purpose: dump paload in hex format
		Serial.printf("\n");
		return;
	}
	if(msg->length+BEEIOT_HDRLEN != packetSize){	// payload == NODEID + GWID + index + cmd + length + statusdata
		Serial.printf("BeeIoTSTatus: Wrong payload size detected (%i) -> package rejected\n", packetSize);
    hexdump((unsigned char*) msg, packetSize); 		// for test purpose: dump paload in hex format
		Serial.printf("\n");
		return;
	}
  // Now we have a valid package: correct ISR Write pointer to next free queue buffer  
  if(RXPkgIsrIdx == MAXRXPKG){  // no next free RX buffer left in sequential queue
    Serial.printf("onReceive: RX Buffer exceeded: switch back to buffer[0]\n");
    RXPkgIsrIdx=0;  // wrap around
  }else{
    RXPkgIsrIdx++;  // simply get next one in line
  }

  // At this point we have a new valid BeeIoT Package
  // in MyRXData[RXPkgIsrIdx-1] or at least at MyRXData[RXPkgSrvIdx]
  BeeIotRXFlag++;   // tell polling service, we have a new package for him

  return;
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