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

#include "BeeIoTWan.h"
#include "beelora.h"    // BeeIoT Lora Message definitions

#include "beeiot.h"     // provides all GPIO PIN configurations of all sensor Ports !



//***********************************************
// Global Sensor Data Objects
//***********************************************
extern unsigned int	lflags;       // BeeIoT log flag field (masks see beeiot.h)
int     islora;                   // =1: we have an active LoRa Node

// GPIO PINS of current connected LoRa Modem chip (SCK, MISO & MOSI are system default)
const int csPin     = BEE_CS;     // LoRa radio chip select
const int resetPin  = BEE_RST;    // LoRa radio reset
const int irqPin    = BEE_DIO0;   // change for your board; must be a hardware interrupt pin

// Lora Modem default configuration
long freq	= FREQ;					        // =EU868_F1..9,DN (EU868_F1: 868.1MHz)
s1_t pw		= TXPOWER;				      // =2-16  TX PA Mode (14)
sf_t sf		= SPREADING+6;  	      // =0..8 Spreading factor FSK,7..12,SFrFu (1:SF7)
bw_t bw		= SIGNALBW;				      // =0..3 RFU Bandwidth 125-500 (0:125kHz)
cr_t cr		= CODING;				        // =0..3 Coding mode 4/5..4/8 (0:4/5)
int	 ih		= IHDMODE;				      // =1 implicite Header Mode (0)
u1_t ihlen	= IHEADERLEN;			    // =0..n if IH -> header length (0)
u1_t nocrc	= NOCRC;				      // =0/1 no CRC check used for Pkg (0)
u1_t noRXIQinversion = NORXIQINV;	// =0/1 flag to switch RX+TX IQinv. on/off (1)

byte localAddress   = NODEID1;    // My address/ID (of this node/device)
byte mygw           = GWID1;      // My current gateway ID to talk with

byte msgCount = 0;        // gobal serial counter of outgoing messages 0..255 round robin
beeiotmsg_t MyMsg;        // Lora message on the air (if MyMsg.data != NULL)

#define MAXRXPKG		  3	  // Max. number of parallel processed RX packages
byte    BeeIotRXFlag =0;            // Semaphor for received message(s) 0 ... MAXRXPKG-1
byte    RXPkgIsrIdx  =0;            // index on next LoRa Package for ISR callback Write
byte    RXPkgSrvIdx  =0;            // index on next LoRa Package for Service Routine Read/serve
beeiotpkg_t MyRXData[MAXRXPKG];     // received message for userland processing

//********************************************************************
// Function Prototypes
//********************************************************************
int  BeeIoTParse	(beeiotpkg_t * msg);
int  sendMessage	(beeiotpkg_t * TXData, int sync);
void onReceive		(int packetSize);

//*********************************************************************
// Setup_LoRa(): init BEE-client object: LoRa
//*********************************************************************
int setup_LoRa(){
    BHLOG(LOGLORAR) Serial.printf("  LoRa: Cfg Lora Modem V%2.1f (%ld Mhz)\n", (float)BEEIOTWAN_VERSION/1000, freq);
    islora = 0;

  // Initial Modem & channel setup:
  // Set GPIO(SPI) Pins, Reset device, Discover SX1276 (only by Lora.h lib) device, results in: 
  // OM=>STDBY/Idle, AGC=On(0x04), freq = FREQ, TX/RX-Base = 0x00, LNA = BOOST(0x03), TXPwr=17
  if (!LoRa.begin(freq)) {          // initialize ratio at LORA_FREQ [MHz] 
    // for SX1276 only (Version = 0x12)
    Serial.println("  LoRa: SX1276 not detected. Check your GPIO connections.");
    return(islora);                          // No SX1276 module found -> Stop LoRa protocol
  }
  // Stream Lora-Regs:      
//   BHLOG(LOGLORAR) LoRa.dumpRegisters(Serial); // dump all SX1276 registers in 0xyy format for debugging

  // BeeIoT-Wan Presets to default LoRa channel (if apart from the class default)
  configLoraModem(/* cfg-struct? */);
  // lets see where we are...
//  BHLOG(LOGLORAR)  LoRa.dumpRegisters(Serial); // dump all SX1276 registers in 0xyy format for debugging

// Setup RX Queue management
  BeeIotRXFlag =0;              // reset Semaphor for received message(s) -> polled by Sendmessage()
  msgCount = 0;                 // no package sent by now
  RXPkgSrvIdx = 0;              // preset RX Queue Read Pointer
  RXPkgIsrIdx = 0;              // preset RX Queue Write Pointer

// Reset TX Msg buffer
  MyMsg.idx=0;
  MyMsg.ack=0;
  MyMsg.retries=0;
  MyMsg.pkg  = (beeiotpkg_t*) NULL;

// Assign IRQ callback on DIO0
  LoRa.onReceive(onReceive);    // (called by loRa.handleDio0Rise(pkglen))
  BHLOG(LOGLORAR) Serial.printf("  LoRa: assign ISR to DIO0  -  GWID:0x%02X, NodeID:0x%02X\n", mygw, localAddress);
  LoRa.sleep();
  BHLOG(LOGLORAR) Serial.printf("\n  Lora: Sleep Mode\n");
  islora=1;                     // Declare:  LORA Modem is ready now!
  return(islora);
} // end of Setup_LoRa()


//***********************************************************************
// Lora Modem Init
// - Can also be used for each channel hopping
// - Modem remains in Idle (standby) mode
//***********************************************************************
void configLoraModem(/* ToDo: cfg struct ? */) {
  long sbw = 0;
  byte coding=0;

  BHLOG(LOGLORAR) Serial.printf("  LoRaCfg: Set Modem/Channel-Cfg: %ldMhz, SF=%i, TXPwr:%i", freq, (byte)sf, (byte)pw);
  LoRa.sleep(); // stop modem and clear FIFO
  // set frequency
  LoRa.setFrequency(freq);

  // RFOUT_pin could be RF_PACONFIG_PASELECT_PABOOST or RF_PACONFIG_PASELECT_RFO
  //   - RF_PACONFIG_PASELECT_PABOOST -- LoRa single output via PABOOST, maximum output 20dBm
  //   - RF_PACONFIG_PASELECT_RFO     -- LoRa single output via RFO_HF / RFO_LF, maximum output 14dBm
  LoRa.setTxPower((byte)pw, PA_OUTPUT_PA_BOOST_PIN); // no boost mode, outputPin = default
  
  // SF6: REG_DETECTION_OPTIMIZE = 0xc5
  //      REG_DETECTION_THRESHOLD= 0x0C

  // SF>6:REG_DETECTION_OPTIMIZE = 0xc3
  //      REG_DETECTION_THRESHOLD= 0x0A
  // implicite setLdoFlag();
  LoRa.setSpreadingFactor((byte)sf);   // ranges from 6-12,default 7 see API docs

  switch (bw) {
        case BW10:  sbw = 10.4E3; break;
        case BW15:  sbw = 15.6E3; break;
        case BW20:  sbw = 20.8E3; break;
        case BW31:  sbw = 31.25E3;break;
        case BW41:  sbw = 41.7E3; break;
        case BW62:  sbw = 62.5E3; break;
        case BW125: sbw = 125E3;  break;
        case BW250: sbw = 250E3;  break;
        case BW500: sbw = 500E3;  break;
        default:  		//  ASSERT(0);
			      Serial.printf("\n    configLoraModem:  Warning_wrong bw setting: %i\n", bw);
  }
  LoRa.setSignalBandwidth(sbw);    // set Signal Bandwidth
  BHLOG(LOGLORAR) Serial.printf(", BW:%ld", sbw);

  switch( cr ) {
        case CR_4_5: coding = 5; break;
        case CR_4_6: coding = 6; break;
        case CR_4_7: coding = 7; break;
        case CR_4_8: coding = 8; break;
		    default:    //  ASSERT(0);
			      Serial.printf("\n    configLoraModem:  Warning_wrong cr setting: %i\n", cr);
        }
  LoRa.setCodingRate4((byte)coding);
  BHLOG(LOGLORAR) Serial.printf(", CR:%i", (byte)coding);

  LoRa.setPreambleLength(LPREAMBLE);    // def: 8Byte
  BHLOG(LOGLORAR) Serial.printf(", LPreamble:%i\n", LPREAMBLE);
  LoRa.setSyncWord(LoRaWANSW);          // ranges from 0-0xFF, default 0x34, see API docs
  BHLOG(LOGLORAR) Serial.printf("    SW:0x%02X", LoRaWANSW);

  if(nocrc){
    BHLOG(LOGLORAR) Serial.print(", NoCRC");
    LoRa.noCrc();                       // disable CRC
  }else{
    BHLOG(LOGLORAR) Serial.print(", CRC");
    LoRa.crc();                         // enable CRC
  }

  if(noRXIQinversion){                  // need RX with Invert IQ 
    BHLOG(LOGLORAR) Serial.print(", noInvIQ");
    LoRa.disableInvertIQ();             // REG_INVERTIQ  = 0x27, REG_INVERTIQ2 = 0x1D
  }else{
    BHLOG(LOGLORAR) Serial.print(", InvIQ\n");
    LoRa.enableInvertIQ();              // REG_INVERTIQ(0x33)  = 0x66, REG_INVERTIQ2(0x3B) = 0x19
  }

  if(ih){
  // LoRa IH Mode initial: 0 -> explicite header Mode ! (needed fp BeeIoTWAN)
  // HeaderMode defined at LoRa.beginPacket(ih) or LoRa.receive(IH-> if size>0) function
  // implicite recognized by LoRa.parse() and LoRa.onreceive(); 
  }

  // Optional: Over Current Protection control -> need here?
  // LoRa.setOCP(?);  // uint8_t 120 or 240 mA

  BHLOG(LOGLORAR) Serial.printf("  LoraCfg: StdBy Mode\n");
  LoRa.idle();                          // goto LoRa Standby Mode
} // end of configLoraModem()


//****************************************************************************************
// LoraLog()
// Converts UserLog-String into LoRa Message format
// Controls flow control: Send -> Wait for Ack ... in case send retries.
// Return:
//    see return codes from BeeIoTParse()
// Send LOG successfull & all RX Queue pkgs processed
// -1:  parsing failed -> message dropped
// -99: timeout waiting for ACK
//****************************************************************************************
int LoRaLog(byte cmd, const char * outgoing, byte outlen, int noack) {
beeiotpkg_t MyTXData;    // Lora package buffer for sending
byte length;
int i;
int rc;

  if(outlen > BEEIOT_DLEN){      // exceeds buffer user header (see below)
    BHLOG(LOGLORAW) Serial.printf("  LoRaLog: data length exceeded, had to cut 0x%02X -> 0x%02X\n", outlen, BEEIOT_DLEN);
    length = BEEIOT_DLEN;        // limit payload length incl. EOL(0D0A)
  }else{  
    length = outlen;             // get real user-payload length
  }
  // Prepare BeeIoT TX package 
  MyTXData.destID = mygw;    // remote target GW
  MyTXData.sendID = localAddress;   // that's me
  MyTXData.index  = msgCount;       // ser. number of sent packages
  MyTXData.cmd    = cmd;            // define type of action/data sent
  MyTXData.length = length;         // length of user payload data incl. any EOL
  strncpy(MyTXData.data, outgoing, length);
  if(cmd == CMD_LOGSTATUS)          // do we send a string ?
    MyTXData.data[length]=0;        // assure EOL = 0

  // remember new Msg (for mutual RETRIES and ACK checks)
  // have to do it before sendmessage -> is used by ISR routine in case of (fast) ACK response
  MyMsg.ack = 0;
  MyMsg.idx = msgCount;
  MyMsg.retries = 0;
  MyMsg.pkg = (beeiotpkg_t*) & MyTXData;

  while(!sendMessage(&MyTXData, 0));   // send it in sync mode
  
  if(noack){ // bypass RX Queue handling -> no wait for ACK requested
    // clean up TX Msg buffer (ISR checks for MyMsg.idx == RXData.index only)
    MyMsg.pkg     = (beeiotpkg_t *) NULL;  // TX done -> release LoRa package buffer
    MyMsg.ack     = 0;
    MyMsg.idx     = 0;
    MyMsg.retries = 0;

    BHLOG(LOGLORAW) Serial.printf("  LoRaLog: Msg(#%i) Done; RX Queue Status SrvIdx:%i, IsrIdx:%i, Length:%i\n", 
            msgCount, RXPkgSrvIdx, RXPkgIsrIdx, BeeIotRXFlag);
    msgCount++;           // increment global sequential TX package/message ID
    rc=0;
    return(rc); // No Ack response expected and we skip waiting for further CMDs
  } 

  // Lets wait for expected ACK Msg & afterwards check RX Queue for some seconds
  // Activate RX Contiguous flow control
  do{                                 // until BeeIoT Message Queue is empty
    LoRa.receive(0);                   // RX Continuous in expl. Header mode          
    BHLOG(LOGLORAW) Serial.printf("  LoRaLog: wait for incoming ACK in RXCont mode (Retry: #%i):", MyMsg.retries);
    i=0;                              // clear RX-ACK wait loop counter
    while(!MyMsg.ack){                // wait till TX got committed by ACK
      BHLOG(LOGLORAW) Serial.print(".");
      delay(MSGBURSTWAIT);            // more time for ACK to arrive -> polling rate

      // Check for ACK Wait Timeout
      if(i++ > MAXRXACKWAIT){                 // max # of wait loops reached ? -> yes, ACK timed out
        BHLOG(LOGLORAW) Serial.printf("timed out\n"); 
        // No ACK -> initiate a retry loop

        if(MyMsg.retries < MSGMAXRETRY){      // enough Retries ?
          while(!sendMessage(&MyTXData, 0));  // No, send same pkg /w same msgid in sync mode again
          MyMsg.retries++;                    // remember # of retries
          LoRa.receive(0);                     // Activate: RX-Continuous in expl. Header mode          
          BHLOG(LOGLORAW) Serial.printf("  LoRaLog: wait for incoming ACK in RXCont mode (Retry: #%i)", MyMsg.retries);
          i=0;  // reset ACK wait loop for next try

        }else{  // Max. # of Retries reached -> give up
          BHLOG(LOGLORAW) Serial.printf("  LoRaLog: Msg(#%i) Send-TimedOut; RX Queue Status: SrvIdx:%i, IsrIdx:%i, Length:%i\n",
                  MyMsg.idx, RXPkgSrvIdx, RXPkgIsrIdx, BeeIotRXFlag);
          // clean up TX Msg buffer (ISR checks for MyMsg.idx == RXData.index only)
          MyMsg.ack     = 0;
          MyMsg.idx     = 0;
          MyMsg.retries = 0;
          MyMsg.pkg     = (beeiotpkg_t *) NULL;  // TX done -> release LoRa package buffer
          BHLOG(LOGLORAR) Serial.printf("\n  LoraLog: Sleep Mode\n");
          LoRa.sleep();
          msgCount++;       // increment global sequ. TX package/message ID for next TX pkg
          rc=-99;           // TX-timeout -> max. # of Retries reached
          return(rc);       // give up and no RX Queue check needed neither -> GW dead ?
        } // if(ACK-TO & Max-Retry) 

      } // if(ACK-TO)
    } // while(no ACK)

    // right after an ACK its time to check for add. RX packages for a while
    MyMsg.ack=0;  // reset ACK & Retry flags again
    MyMsg.retries=0;
    LoRa.receive(0);                     // Activate: RX-Continuous in expl. Header mode          

    i=0;
    BHLOG(LOGLORAW) Serial.printf("\n  LoRaLog: wait for add. RX Pkg. in RXCont mode:");
    while (!BeeIotRXFlag & (i<WAITRXPKG*2)){  // till RX pkg arrived or max. wait time is over
      BHLOG(LOGLORAW) Serial.print("o");
      delay(500);            // count wait time in msec. -> frequency to check RXQueue
      i++;
    } 

    // reached TO condition ?
    if(i>=WAITRXPKG){ // Yes, no more RX pkg received -> we are done
      // nothing else to do: ACK was solved by ISR completely (no RX Queue handling needed)
      BHLOG(LOGLORAW) Serial.println(" None.\n");
      // RX Queue should still be empty: validate it:
      if(!BeeIotRXFlag & (RXPkgSrvIdx != RXPkgIsrIdx)){ // RX Queue validation check: realy empty ?
        Serial.printf("  LoRaLog: This case should never happen 1: Queue-RD(%i)/WR(%i) Index different when BeeIoTRXFlag==0\n",RXPkgSrvIdx, RXPkgIsrIdx);
      }
      rc=0;
    }else{  // new pkg received -> process it
      // ISR has checked Header and copied pkg into MyRXData[RXPkgSrvIdx] -> now parse it
      BHLOG(LOGLORAR) Serial.printf("  LoRaLog: add. Pkg received: %s\n", beeiot_ActString[MyRXData[RXPkgSrvIdx].cmd]); 
      BHLOG(LOGLORAR) Serial.printf("  LoRaLog: RX Queue Status: SrvIdx:%i, IsrIdx:%i, new PkgID:%i, RXFlag:%i\n",
              RXPkgSrvIdx, RXPkgIsrIdx, MyRXData[RXPkgSrvIdx].index, BeeIotRXFlag); 

      rc = BeeIoTParse(&MyRXData[RXPkgSrvIdx]); // parse next MyRXData[RXPkgSrvIdx] for action 
      if(rc < 0){
        BHLOG(LOGLORAW) Serial.printf("  LoRaLog: parsing of Msg[%i]failed rc=%i\n", MyRXData[RXPkgSrvIdx].index, rc);
        // Detailed error analysis/message done by BeeIoTParse() already
      }
      // if RC == RETRY -> loop for ACK-wait again

      if(++RXPkgSrvIdx >= MAXRXPKG){  // no next free RX buffer left in sequential queue
        BHLOG(LOGLORAR) Serial.printf("  LoRaLog: RX Buffer end reached, switch back to buffer[0]\n");
        RXPkgSrvIdx=0;  // wrap around
      }
      BeeIotRXFlag--;   // and we can release one more RX Queue buffer
      if(!BeeIotRXFlag & (RXPkgSrvIdx != RXPkgIsrIdx)){ // RX Queue validation check: realy empty ?
        Serial.printf("  LoRaLog: This case should never happen 2: Queue-RD(%i)/WR(%i) Index different when BeeIoTRXFlag==0\n",RXPkgSrvIdx, RXPkgIsrIdx);
      }

    } // New Pkg parsed
  } while(rc == CMD_RETRY); // for this CMDs do ACK wait again

  // clean up TX Msg buffer (ISR checks for MyMsg.idx == RXData.index only)
  MyMsg.ack     = 0;
  MyMsg.idx     = 0;
  MyMsg.retries = 0;
  MyMsg.pkg     = (beeiotpkg_t *) NULL;  // TX done -> release LoRa package buffer

  BHLOG(LOGLORAR) Serial.printf("\n  LoraLog: Sleep Mode\n");
  LoRa.sleep();

  msgCount++;           // increment global sequ. TX package/message ID
  BHLOG(LOGLORAR) Serial.printf("  LoRaLog: Msg sent done, RX Queue Status: SrvIdx:%i, IsrIdx:%i, NextMsgID:%i, RXFlag:%i\n", RXPkgSrvIdx, RXPkgIsrIdx, msgCount, BeeIotRXFlag); 

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
		// intend to do nothing -> test message for communication check
		// for test purpose: dump payload in hex format
    BHLOG(LOGLORAW) Serial.printf("  BeeIoTParse[%i]: cmd= NOP received\n", msg->index); 
		rc= CMD_NOP;	
		break;

	case CMD_CONFIG:                           // new BeeIoT channel cfg received
    BHLOG(LOGLORAW) Serial.printf("  BeeIoTParse[%i]: cmd= CONFIG new channel cfg. data stored\n", msg->index); 
    // ToDo: action: parse channel cg. data and update channel cfg. table
    rc=CMD_CONFIG;                          // -> wait for ACK by upper layer required
		break;

	case CMD_LOGSTATUS: // BeeIoT node sensor data set received
    BHLOG(LOGLORAW) Serial.printf("  BeeIoTParse[%i]: cmd= LOGSTATUS -> should be ACK => ignored\n", msg->index); 
    // intentionally do nothing on node side
    rc=CMD_LOGSTATUS;
		break;

	case CMD_GETSDLOG:
    BHLOG(LOGLORAW) Serial.printf("  BeeIoTParse[%i]: SDLOG Save command: not supported yet\n", msg->index); 
		// for test purpose: dump payload in hex format
			// ToDo: read out SD card and send it package wise to GW
		rc= CMD_GETSDLOG;                      // not supported yet
		break;

	case CMD_RETRY:
    BHLOG(LOGLORAW) Serial.printf("  BeeIoTParse[%i]: RETRY-Request: send same message again\n", msg->index); 
    if(MyMsg.pkg->index == msg->index){    // do we talk about the same Node-TX message we are waiting for ?
      rc=CMD_RETRY;                        // -> wait for ACK by upper layer required
    }else{
      BHLOG(LOGLORAW) Serial.printf("  BeeIoTParse: But new MsgID[%i] does not match to TXpkg[%i] ->ignored\n", msg->index, MyMsg.pkg->index);
      rc = -1;  // retry requested but not for last sent message -> obsolete request
    }
		break;

	case CMD_ACK:   // we should not reach this point
    BHLOG(LOGLORAW) Serial.printf("  BeeIoTParse[%i]: ACK received -> Should have been done by ISR ?!\n", msg->index); 
    //  Already done by onReceive() implicitely
      // BHLOG(LOGLORAW) Serial.printf("  BeeIoTParse[%i]: ACK received -> Data transfer done.\n", msg->index); 
      if(MyMsg.pkg->index == msg->index){    // save link to User payload data
        MyMsg.ack = 1;                        // confirm Acknowledge flag
      }
    rc = CMD_ACK;   // Message sending got acknowledged
		break;

	default:
    BHLOG(LOGLORAW) Serial.printf("  BeeIoTParse[%i]: unknown CMD(%d)\n", msg->index, msg->cmd); 
		// for test purpose: dump paload in hex format
    BHLOG(LOGLORAW) hexdump((unsigned char*) msg, msg->length+BEEIOT_HDRLEN);		
		rc= -5;
		break;
	}
	
	return(rc);
}


//****************************************************************************************
// SendMessage()
// Converts UserLog-String into LoRa Message format
// Controls flow control: Send -> Wait for Ack ... in case send retries.
// Input:
//  TXData  User payload structure to be sent
//  Sync    =0 sync mode: wait for TX Done flag
//          =1 async mode: return immediately after TX -> no wait for TXDone
// return:
//    0: Modem still in TX mode -> waiting for TXDone
//    1: data was sent successfully
//****************************************************************************************
int sendMessage(beeiotpkg_t * TXData, int async) {

  configLoraModem();  // setup channel for next send action

// start package creation:
  if(!LoRa.beginPacket(0))            // reset FIFO ptr.; in explicit header Mode
    return(0);                         // still transmitting -> have to come back later

  Serial.flush(); // send all Dbg msg up to here...

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
  LoRa.endPacket(async);

  BHLOG(LOGLORAR) Serial.printf("  LoRaSend(0x%x>0x%x)[%i](cmd=%d)%s - %iBy\n", TXData->sendID, TXData->destID, TXData->index, TXData->cmd, TXData->data, TXData->length+BEEIOT_HDRLEN);
  BHLOG(LOGLORAR) hexdump((unsigned char*) TXData, TXData->length + BEEIOT_HDRLEN);
  return(1);
}


//******************************************************************************
// IRQ User callback function 
// => package received (called by LoRa.handleDio0Rise() )
// Input:
//  packetSize  length of recognized RX package user payload in FIFO
//******************************************************************************
void onReceive(int packetSize) {
// at that point all IRQ flags are comitted, CRC check done, Header Mode processed and
// FIFO ptr (REG_FIFO_ADDR_PTR) was reset to package start
// simply use LoRa.read() to read out the FIFO
beeiotpkg_t * msg;
byte count;
byte * ptr;

	  BHLOG(LOGLORAR) Serial.printf("\nonReceive: got Pkg: len 0x%02X\n", packetSize);

    if(BeeIotRXFlag >= MAXRXPKG){ // Check RX Semaphor: RX-Queue full ?
    // same situation as: RXPkgIsrIdx+1 == RXPkgSrvIdx (but hard to check with ring buffer)
    Serial.printf("onReceive: InQueue full (%i items)-> ignore new IRQ with Len 0x%x\n", (byte) BeeIotRXFlag, (byte) packetSize);
    return;     // User service must work harder
  }

  // is it a package size conform to BeeIoT WAN definitions ?
  if (packetSize < BEEIOT_HDRLEN || packetSize > MAX_PAYLOAD_LENGTH) {
    Serial.printf("onReceive: New RXPkg exceed BeeIoT pkg size: len=0x%x\n", packetSize);
    return;          // no payload of interest for us, ignore it.
  }

  // from here on: assumed explicite header mode: -> Size from = REG_RX_NB_BYTES Bytes
  // read expected packet user-header bytes leading BeeIoT-WAN header
  ptr = (byte*) & MyRXData[RXPkgIsrIdx];

  for(count=0; count < packetSize; count++){      // as long as FiFo Data bytes left
    ptr[count] = (byte)LoRa.read();     // read bytes one by one for REG_RX_NB_BYTES Bytes
  }
  msg = & MyRXData[RXPkgIsrIdx];  // Assume: BeeIoT-WAN Message received
  BHLOG(LOGLORAR) hexdump((unsigned char*) msg, BEEIOT_HDRLEN);
	
// now check real data:
// start the gate keeper of valid BeeIoT Package data:
	if(msg->sendID != mygw){
		Serial.printf("onReceive: Unknown NodeID detected (0x%02X) -> package rejected\n", (unsigned char)msg->sendID);		
		return;
	}
	if(msg->destID != localAddress){
		Serial.printf("onReceive: Wrong Target GWID detected (0x%02X) -> package rejected\n", (unsigned char)msg->destID);		
		BHLOG(LOGLORAR) Serial.println();
		return;
	}
	if(msg->length+BEEIOT_HDRLEN != packetSize){	// payload == NODEID + GWID + index + cmd + length + statusdata
		Serial.printf("onReceive: Wrong payload size detected (%i) -> package rejected\n", packetSize);
	  BHLOG(LOGLORAR) Serial.println();
		return;
	}

  // At this point we have a new header-checked BeeIoT Package
  // some debug output: assumed all is o.k.
	BHLOG(LOGLORAW) Serial.printf("\nonReceive: RX(0x%02X>0x%02X)", (unsigned char)msg->sendID, (unsigned char)msg->destID);
	BHLOG(LOGLORAW) Serial.printf("[%i]:(cmd=%i: %s) ", (unsigned char) msg->index, (unsigned char) msg->cmd, beeiot_ActString[msg->cmd]);
  BHLOG(LOGLORAW) Serial.printf("DataLen=%i Bytes\n", msg->length);

  // if it is CMD_ACK pkg -> lets commit to waiting TX-Msg
  if(msg->cmd == CMD_ACK){
    if(msg->index == MyMsg.idx){  // is a TX task waiting for this ACK ?
      MyMsg.ack = 1;              // ACK flag hopefully still polled by waiting TX service
    }else{                        // noone is waiting for this ACK -> ignore it, but leave a note
      BHLOG(LOGLORAR) Serial.printf("onReceive: ACK received -> with no matching msg.index: %i\n", msg->index);
    } // skip RX Queue handling for ACK pkgs.

  }else{  // for any other CMD of RX package -> lets do it on userland side
    //  Correct ISR Write pointer to next free queue buffer  
    //  in MyRXData[RXPkgIsrIdx-1] or at least at MyRXData[RXPkgSrvIdx]
    if(++RXPkgIsrIdx >= MAXRXPKG){  // no next free RX buffer left in sequential queue
      BHLOG(LOGLORAR) Serial.printf("onReceive: RX Buffer exceeded: switch back to buffer[0]\n");
      RXPkgIsrIdx=0;  // wrap around
    }
    BeeIotRXFlag++;   // tell parsing service, we have a new package for him
  }

  Serial.flush(); // print ISR notes in real time
  return;
} // end of RX ISR process





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