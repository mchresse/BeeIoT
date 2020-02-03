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
#include <string.h>

#include <LoRa.h>       // from LoRa Sandeep Driver 

#include "BeeIoTWan.h"
#include "beelora.h"    // BeeIoT Lora Message definitions

#include "beeiot.h"     // provides all GPIO PIN configurations of all sensor Ports !



//***********************************************
// Global Sensor Data Objects
//***********************************************
// Central Database of all measured values and runtime parameters
extern dataset		bhdb;           // central node status DB -> main.cpp
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

//////////////////////////////////////////////////
// CONFIGURATION (FOR APPLICATION CALLBACKS BELOW)
//////////////////////////////////////////////////
// application router ID (LSBF)
  u1_t JOINEUI[8]  = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08 };  // aka APPEUI
// unique device ID (LSBF)
  u1_t DEVEUI[8]  = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08 };
// device-specific AES key (derived from device EUI)
  u1_t DEVKEY[16] = { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F};

byte localAddress   = NODEID1;    // My address/ID (of this node/device)
byte mygw           = GWID1;      // My current gateway ID to talk with

byte BeeIoTStatus = BIOT_NONE;    // Current Status of BeeIoT WAN protocol (not OPMode !) -> beelora.h
byte msgCount = 0;        // gobal serial counter of outgoing messages 0..255 round robin
beeiotmsg_t MyMsg;        // Lora message on the air (if MyMsg.data != NULL)

#define MAXRXPKG		  3	  // Max. number of parallel processed RX packages
byte    BeeIotRXFlag =0;            // Semaphor for received message(s) 0 ... MAXRXPKG-1
byte    RXPkgIsrIdx  =0;            // index on next LoRa Package for ISR callback Write
byte    RXPkgSrvIdx  =0;            // index on next LoRa Package for Service Routine Read/serve
beeiotpkg_t MyRXData[MAXRXPKG];     // received message for userland processing
beeiotpkg_t MyTXData;               // Lora package buffer for sending

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
    BHLOG(LOGLORAR) Serial.printf("  LoRa: Cfg Lora Modem V%2.1f (%ld Mhz)\n", (float)BIoT_VERSION/1000, freq);
    islora = 0;
    BeeIoTStatus = BIOT_NONE;

  // Initial Modem & channel setup:
  // Set GPIO(SPI) Pins, Reset device, Discover SX1276 (only by Lora.h lib) device, results in: 
  // OM=>STDBY/Idle, AGC=On(0x04), freq = FREQ, TX/RX-Base = 0x00, LNA = BOOST(0x03), TXPwr=17
  if (!LoRa.begin(freq)) {          // initialize ratio at LORA_FREQ [MHz] 
    // for SX1276 only (Version = 0x12)
    Serial.println("  LoRa: SX1276 not detected. Check your GPIO connections.");
    return(islora);                          // No SX1276 module found -> Stop LoRa protocol
  }
  // For debugging: Stream Lora-Regs:      
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
  BHLOG(LOGLORAR) Serial.printf("  LoRa: assign ISR to DIO0  - default: GWID:0x%02X, NodeID:0x%02X\n", mygw, localAddress);
  islora=1;                     // Declare:  LORA Modem is active now!

  // From now on : JOIN to a GW
  BeeIoTStatus = BIOT_JOIN; // have to join to a GW

  // 1. Try: send a message to join to a GW
  if(BeeIoTJoin() <= 0){
    BHLOG(LOGLORAW)  Serial.printf("  Lora: 1. BeeIoT JOIN failed -> remaining in BIOT_JOIN Mode\n");
    // ToDo: any Retry action after a while ?
    BeeIoTStatus = BIOT_JOIN;
  }else{
    BeeIoTStatus = BIOT_IDLE; // we have a joined modem -> wait for RX/TX pkgs.

    // Modem is joined and ready, but may be BIOT_SLEEP would be better to save power
    // BeeIoTSleep();
  }

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
// BeeIoTJoin()
// Discocers GW session providing Node identification as LoRa JOIN message.
// Expects in turn a CMD_CONFIG msg for future channel configuration.
// Global: MyTXData[] casted by pjoin ptr.
// Return:
//   1:  JOIN was successfull GW channel config received and modem is resconfigured
//       -> New BeeIoT_Status = BIOT_IDLE
//   0:  no joined GW found  -> BeeIoTStatus = BIOT_JOIN
//  -1:  JOIN failed -> TO or wrong channel cfg.
//  -2:  Max. # of retries
//****************************************************************************************
int BeeIoTJoin(void){
beeiot_join_t * pjoin;  // ptr. on JOIN message format field (reusing global MyTXData buffer)
int i;
int rc;

pjoin = (beeiot_join_t *) & MyTXData; // fetch global Msg buffer
BHLOG(LOGLORAW) Serial.printf("  BeeIoTJoin: Start Joining for a GW\n");

 // Prepare BeeIoT JOIN package 
  pjoin->hd.destID = GWID1;         // We start with default BeeIoT-WAN GWID
  pjoin->hd.sendID = localAddress;  // that's me by now but finally not checked in JOIN session
  pjoin->hd.index  = 0xFF;          // ser. number of JOIN package; well, could be any ID 0..0xFF
  pjoin->hd.cmd    = CMD_JOIN;      // Lets Join
  pjoin->hd.frmlen = sizeof(joinpar_t); // length of JOIN payload data
  pjoin->mic[0]    = 0;             // ToDo: MIC generation 

  // remember new Msg (for possible RETRIES)
  // have to do it before sendmessage -> is used by ISR routine in case of (fast) ACK response
  MyMsg.ack = 0;                    // no ACK expected, but a CONFIG pkg
  MyMsg.idx = pjoin->hd.index;      // needed to match corresponding CONFIG pkg
  MyMsg.retries = 0;
  MyMsg.pkg = (beeiotpkg_t*) & MyTXData;  // and pkg data itself
  
  BeeIoTStatus = BIOT_JOIN;          // start TX_JOIN session
  while(!sendMessage(&MyTXData, 0));  // send it in sync mode

  // Lets wait for expected CONFIG Msg
  // Activate RX Contiguous flow control
  do{
    MyMsg.ack=0;                // reset ACK & Retry flags again
    LoRa.receive(0);            // Activate: RX-Continuous in expl. Header mode          

    i=0;  // reset wait counter
    BHLOG(LOGLORAW) Serial.printf("  BeeIoTJoin: waiting for RX-CONFIG Pkg. in RXCont mode:");
    while (!BeeIotRXFlag & (i<WAITRX1PKG*2)){  // till RX pkg arrived or max. wait time is over
        BHLOG(LOGLORAW) Serial.print("o");
        delay(500);            // count wait time in msec. -> frequency to check RXQueue
        i++;
    } 

    rc=0;
    if(i>=WAITRX1PKG){       // TO condition reached, no RX pkg received=> no GW in range ?
        BHLOG(LOGLORAW) Serial.println(" None.\n");
        // RX Queue should still be empty: validate it:
        if(!BeeIotRXFlag & (RXPkgSrvIdx != RXPkgIsrIdx)){ // RX Queue validation check: realy empty ?
          Serial.printf("  BeeIotJoin: This case should never happen: Queue-RD(%i)/WR(%i) Index different when BeeIoTRXFlag==0\n",RXPkgSrvIdx, RXPkgIsrIdx);
          // ToDo: any correction action ? or exit ?
          RXPkgSrvIdx = RXPkgIsrIdx;  // no Queue entry: RD & WR ptr. must be identical
        }
        rc=CMD_RETRY;       // initiate JOIN request again
    }else{  // RX pkg received
      // ISR has checked Header and copied pkg into MyRXData[RXPkgSrvIdx] already -> now parse it
      BHLOG(LOGLORAR) Serial.printf("  BeeIotJoin: RX pkg received: %s\n", beeiot_ActString[MyRXData[RXPkgSrvIdx].hd.cmd]); 
      BHLOG(LOGLORAR) Serial.printf("  BeeIotJoin: RX Queue Status: SrvIdx:%i, IsrIdx:%i, new PkgID:%i, RXFlag:%i\n",
              RXPkgSrvIdx, RXPkgIsrIdx, MyRXData[RXPkgSrvIdx].hd.index, BeeIotRXFlag); 

      rc = BeeIoTParse(&MyRXData[RXPkgSrvIdx]); // parse MyRXData[RXPkgSrvIdx] for action 
      if(rc < 0){
        BHLOG(LOGLORAW) Serial.printf("  BeeIotJoin: parsing of Msg[%i]failed rc=%i\n", MyRXData[RXPkgSrvIdx].hd.index, rc);
        // Detailed error analysis/message done by BeeIoTParse() already
        rc=CMD_RETRY;
      }
      // release RX Queue buffer
      if(++RXPkgSrvIdx >= MAXRXPKG){  // no next free RX buffer left in sequential queue
        BHLOG(LOGLORAR) Serial.printf("  BeeIotJoin: RX Buffer end reached, switch back to buffer[0]\n");
        RXPkgSrvIdx=0;  // wrap around
      }
      BeeIotRXFlag--;   // and we can release one more RX Queue buffer
      if((!BeeIotRXFlag) & (RXPkgSrvIdx != RXPkgIsrIdx)){ // RX Queue validation check: realy empty ?
        Serial.printf("  BeeIotJoin: This case should never happen 2: Queue-RD(%i)/WR(%i) Index different when BeeIoTRXFlag==0\n",RXPkgSrvIdx, RXPkgIsrIdx);
      }
    } // New Pkg parsed

    if(rc==CMD_RETRY){ // it is requested to send JOIN again (because either no or wrong response or explicitly requested by GW)
       if(MyMsg.retries < MSGMAXRETRY){       // enough Retries ?
          BeeIoTStatus = BIOT_JOIN;           // No , start TX session
          while(!sendMessage(&MyTXData, 0));  // Send same pkg /w same msgid in sync mode again
          MyMsg.retries++;                    // remember # of retries
          BHLOG(LOGLORAW) Serial.printf("  BeeIotJoin: JOIN-Retry: #%i\n", MyMsg.retries);
          i=0;  // reset ACK wait loop for next try

        }else{  // Max. # of Retries reached -> give up
          BHLOG(LOGLORAW) Serial.printf("  BeeIotJoin: JOIN-Request stopped (Retries: #%i)\n", MyMsg.retries);
          // clean up TX Msg buffer (ISR checks for MyMsg.idx == RXData.index only)
          MyMsg.ack     = 0;
          MyMsg.idx     = 0;
          MyMsg.retries = 0;
          MyMsg.pkg     = (beeiotpkg_t *) NULL;  // TX done -> release LoRa package buffer
          BeeIoTStatus  = BIOT_JOIN;             // remain in joining mode
//          BeeIoTSleep();
//          msgCount++;       // increment global sequ. TX package/message ID for next TX pkg
          rc=-2;           // TX-timeout -> max. # of Retries reached
          return(rc);       // give up and no RX Queue check needed neither -> GW dead ?
        } // if(Max-Retry) 
    } // Retry action
    if(rc==CMD_CONFIG){
      // we are joined with GW
      BeeIoTStatus = BIOT_IDLE;   // we are connected
      rc=1; // leave this loop
    }
  } while(rc == CMD_RETRY); // Retry JOINing again

  if(BeeIoTStatus==BIOT_IDLE){
    BHLOG(LOGLORAR) Serial.printf("  Lora: Joined! New: GWID:0x%02X, NodeID:0x%02X\n", mygw, localAddress);
    BHLOG(LOGLORAR) Printhex( DEVEUI,  8, "    DEVEUI: 0x-"); Serial.println();
    BHLOG(LOGLORAR) Printhex( JOINEUI, 8, "   JOINEUI: 0x-"); Serial.println();   // == APPEUI
    BHLOG(LOGLORAR) Printhex( DEVKEY, 16, "    DEVKEY: 0x-", 2); Serial.println();
    BHLOG(LOGLORAR) Printhex( (u1_t*) & bhdb.BoardID, 6, "   BoardID: 0x-", 6, 1); Serial.print(" -> ");
    BHLOG(LOGLORAR) Printbit( (u1_t*) & bhdb.BoardID, 6*8, "  0b-", 1, 1); Serial.println();
    LoRa.idle();
    rc=1;
  }
 return(rc);
}

//*************************************************************************
// PrintHex()
// Print bin field: pbin[0] <-> pbin[len-1] by given direction in hexadez.
// dump format: '0x-xx-xx-xx-xx-xx....'
// Print starts where cursor is and no EOL is used.
// INPUT
//    pbin    byte ptr on binary field[0]
//    bytelen number of 2 digit bytes to be printed
// (0)dir     =0 -> forward  [0...len-1],   =1 -> backward [len-1...0]
// (1)format  =1: bytewise  =2: 2bytes(short)  =4bytes(word) =8bytes(long)...
//*************************************************************************
void Printhex(unsigned char * pbin, int bytelen, const char * prefix, int format, int dir ){
// dir=0 > forward; dir=1 -> backward
int c; int bytype;

  if (pbin && bytelen) {   // prevent NULL ptr. and bitlen=0
    bytype = format;
    if(dir<0 || dir>1) {
      Serial.println("printHex(): 'dir' must be 0 or 1");
      return;  // check dir-marker range
    }
    Serial.print(prefix);
    for(c=(bytelen-1)*dir; c!=(bytelen*(1-dir)-dir); c=c+1-(2*dir)){
      if(!bytype){
        Serial.printf("-");
        bytype = format;
      }
      Serial.printf("%02X", pbin[c]);
      bytype--;
    }
  }
}

//*************************************************************************
// PrintBit()
// Print binary field: pbin[0] <-> pbin[len-1] by given direction in 0/1 digits
// dump format: '0b-bbbbbbbb-bbbbbbbb-...'  e.g.  0b-10010110-10101100 (dir=0)
//                  76543210-76543210                 0x96      0xAC  (forward)
// Print starts where cursor is and no EOL is used.
// INPUT
//    pbin    byte ptr on binary field[0]
//    bitlen  number of bits (!) to be printed in bit stream format
// (0)dir     =0 -> forward  [0...len-1],   =1 -> backward [len-1...0]
// (1)format  =1: bytewise  =2: 2bytes(short)  =4bytes(word) =8bytes(long)...
//*************************************************************************
void Printbit(unsigned char * pbin, int bitlen, const char * prefix, int format, int dir){
int c; int bit; int len;
int bytype;

  if (pbin && bitlen) {   // prevent NULL ptr. and bitlen=0
    if(dir<0 || dir>1) {
      Serial.println("PrintBit(): 'dir' must be 0 or 1");
      return;  // check dir-marker range
    }
    len = bitlen/8;     // get byte counter
    if(len*8 < bitlen)  // remaining bits after last full byte ?
      len++;            // we have to do byteloop one more time
    bytype=format;
    Serial.print(prefix);
    for(c=(len-1)*dir; c!=(len*(1-dir)-dir); c=c+1-(2*dir)){      // byte loop forw./backw. 
      if(!bytype){
        Serial.print("-");
        bytype=format;
      }
      for(bit=7*dir; bit!=8*(1-dir)-dir; bit=bit+1-(2*dir)){      // bit  loop forw./backw.
        Serial.printf("%c", pbin[c] & (1u << bit) ? '1' : '0');
        if(!(--bitlen))   // last requested bit printed ?
          return;
      }
      bytype--;
    }
  }
} // end of Printbit()


int BeeIoTWakeUp(void){
  BHLOG(LOGLORAR) Serial.printf("  BeeIoTWakeUp()\n");
  // toDo. kind of Setup_Lora here
  configLoraModem();
  BeeIoTStatus = BIOT_IDLE;     // modem is active
  return(islora);
}

void BeeIoTSleep(void){
    BHLOG(LOGLORAR) Serial.printf("  BeeIoTSleep()\n");
    BeeIoTStatus = BIOT_SLEEP;  // go into power safe mode
    BHLOG(LOGLORAR) Serial.printf("  Lora: Sleep Mode\n");
    LoRa.sleep();
}

//****************************************************************************************
// LoraLog()
// Converts UserLog-String into LoRa Message format
// Controls flow control: Send -> Wait for Ack ... in case send retries.
// Return:
//    see return codes from BeeIoTParse()
// Send LOG successfull & all RX Queue pkgs processed
// -1:  parsing failed -> message dropped
// -96:  no joined GW found  -> BeeIoTStatus = BIOT_JOIN
// -97:  modem still sleeping; can't wake up
// -98:  no cfg. modem found
// -99: timeout waiting for ACK
//****************************************************************************************
int LoRaLog(byte cmd, const char * outgoing, byte outlen, int noack) {
byte length;
int i;
int rc;
  BHLOG(LOGLORAR) Serial.printf("  LoRaLog: BeeIoTStatus = %i\n", BeeIoTStatus);
  if(BeeIoTStatus == BIOT_NONE){ 
    if(!setup_LoRa())
      return(-98);
  }
  if(BeeIoTStatus == BIOT_SLEEP) {
    if(!BeeIoTWakeUp()) // we have to wakeup first
      return(-97);
  }

  if(BeeIoTStatus == BIOT_JOIN){  // still need a joined GW before sending anything
    if(BeeIoTJoin() <= 0){
      BHLOG(LOGLORAW)  Serial.printf("  LoraLog: BeeIoT JOIN failed -> skipped LoRa Logging\n");
      // ToDo: any Retry action after a while ? By now we simply stop logging
      return(-96);
    }
  }
  // continue to prepare LogStatus package
  if(outlen > BIoT_FRAMELEN){      // exceeds frame buffer  space (see below)?
    BHLOG(LOGLORAW) Serial.printf("  LoRaLog: data length exceeded, had to cut 0x%02X -> 0x%02X\n", outlen, BIoT_FRAMELEN);
    length = BIoT_FRAMELEN;        // limit payload length incl. EOL(0D0A)
  }else{  
    length = outlen;             // get real user-payload length
  }
  // Prepare BeeIoT TX package 
  MyTXData.hd.destID = mygw;    // remote target GW
  MyTXData.hd.sendID = localAddress;   // that's me
  MyTXData.hd.index  = msgCount;       // ser. number of sent packages
  MyTXData.hd.cmd    = cmd;            // define type of action/data sent
  MyTXData.hd.frmlen = length;         // length of user payload data incl. any EOL
  strncpy(MyTXData.data, outgoing, length);
  if(cmd == CMD_LOGSTATUS)          // do we send a string ?
    MyTXData.data[length]=0;        // assure EOL = 0

  // remember new Msg (for mutual RETRIES and ACK checks)
  // have to do it before sendmessage -> is used by ISR routine in case of (fast) ACK response
  MyMsg.ack = 0;
  MyMsg.idx = msgCount;
  MyMsg.retries = 0;
  MyMsg.pkg = (beeiotpkg_t*) & MyTXData;
  
  BeeIoTStatus = BIOT_TX;  // start TX session
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
      delay(MSGRESWAIT);            // more time for ACK to arrive -> polling rate

      // Check for ACK Wait Timeout
      if(i++ > MAXRXACKWAIT){                 // max # of wait loops reached ? -> yes, ACK timed out
        BHLOG(LOGLORAW) Serial.printf("timed out\n"); 
        // No ACK -> initiate a retry loop

        if(MyMsg.retries < MSGMAXRETRY){      // enough Retries ?
          BeeIoTStatus = BIOT_TX;  // start TX session
          while(!sendMessage(&MyTXData, 0));  // No, send same pkg /w same msgid in sync mode again
          MyMsg.retries++;                    // remember # of retries
          LoRa.receive(0);                    // Activate: RX-Continuous in expl. Header mode          
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
          BeeIoTStatus = BIOT_IDLE; // release modem for other sessions

          BHLOG(LOGLORAR) Serial.printf("\n  LoraLog: Sleep Mode\n");
          BeeIoTSleep();
          msgCount++;       // increment global sequ. TX package/message ID for next TX pkg
          rc=-99;           // TX-timeout -> max. # of Retries reached
          return(rc);       // give up and no RX Queue check needed neither -> GW dead ?
        } // if(ACK-TO & Max-Retry) 

      } // if(ACK-TO)
    } // while(no ACK)

    // right after an ACK its time to check for add. RX packages for a while
    BeeIoTStatus = BIOT_RX;    // enter RX1 session
    MyMsg.ack=0;                // reset ACK & Retry flags again
    MyMsg.retries=0;
    LoRa.receive(0);            // Activate: RX-Continuous in expl. Header mode          

    i=0;  // reset wait counter
    BHLOG(LOGLORAW) Serial.printf("\n  LoRaLog: wait for add. RX1 Pkg. (RXCont):");
    while (!BeeIotRXFlag & (i<WAITRX1PKG*2)){  // till RX pkg arrived or max. wait time is over
      BHLOG(LOGLORAW) Serial.print("o");
      delay(500);            // count wait time in msec. -> frequency to check RXQueue
      i++;
    } 

    // reached TO condition ?
    if(i>=WAITRX1PKG){ // Yes, no more RX pkg received -> we are done
      // nothing else to do: ACK was solved by ISR completely (no RX Queue handling needed)
      BHLOG(LOGLORAW) Serial.println(" None.\n");
      // RX Queue should still be empty: validate it:
      if(!BeeIotRXFlag & (RXPkgSrvIdx != RXPkgIsrIdx)){ // RX Queue validation check: realy empty ?
        Serial.printf("  LoRaLog: This case should never happen 1: Queue-RD(%i)/WR(%i) Index different when BeeIoTRXFlag==0\n",RXPkgSrvIdx, RXPkgIsrIdx);
        // ToDo: any correction action ? or exit ?
        RXPkgSrvIdx = RXPkgIsrIdx;  // no Queue entry: RD & WR ptr. must be identical
      }
      rc=0; // anyway, we leave RX Queue scanning -> has to be empty
    }else{  // new pkg received -> process it
      // ISR has checked Header and copied pkg into MyRXData[RXPkgSrvIdx] -> now parse it
      BHLOG(LOGLORAR) Serial.printf("  LoRaLog: add. Pkg received: %s\n", beeiot_ActString[MyRXData[RXPkgSrvIdx].hd.cmd]); 
      BHLOG(LOGLORAR) Serial.printf("  LoRaLog: RX Queue Status: SrvIdx:%i, IsrIdx:%i, new PkgID:%i, RXFlag:%i\n",
              RXPkgSrvIdx, RXPkgIsrIdx, MyRXData[RXPkgSrvIdx].hd.index, BeeIotRXFlag); 

      rc = BeeIoTParse(&MyRXData[RXPkgSrvIdx]); // parse next MyRXData[RXPkgSrvIdx] for action 
      if(rc < 0){
        BHLOG(LOGLORAW) Serial.printf("  LoRaLog: parsing of Msg[%i]failed rc=%i\n", MyRXData[RXPkgSrvIdx].hd.index, rc);
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

  BeeIoTStatus = BIOT_IDLE;  // free for new jobs

   // clean up TX Msg buffer (ISR checks for MyMsg.idx == RXData.index only)
  MyMsg.ack     = 0;
  MyMsg.idx     = 0;
  MyMsg.retries = 0;
  MyMsg.pkg     = (beeiotpkg_t *) NULL;  // TX done -> release LoRa package buffer
  msgCount++;           // increment global sequ. TX package/message ID

  BHLOG(LOGLORAR) Serial.printf("\n  LoraLog: Enter Sleep Mode\n");
  BeeIoTSleep();

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
	switch (msg->hd.cmd){
	case CMD_NOP:
		// intended to do nothing -> test message for communication check
		// for test purpose: dump payload in hex format
    BHLOG(LOGLORAW) Serial.printf("  BeeIoTParse[%i]: cmd= NOP received\n", msg->hd.index); 
		rc= CMD_NOP;	
		break;

	case CMD_JOIN:
		// intended to do nothing -> typically send from Node -> GW only.
    BHLOG(LOGLORAW) Serial.printf("  BeeIoTParse[%i]: cmd= JOIN received from sender: 0x%02X\n", msg->hd.index, msg->hd.sendID);
		rc= CMD_JOIN;	
		break;

	case CMD_CONFIG:                           // new BeeIoT channel cfg received
    BHLOG(LOGLORAW) Serial.printf("  BeeIoTParse[%i]: cmd= CONFIG -> switch to new channel cfg.\n", msg->hd.index); 
    // ToDo: action: parse channel cg. data and update channel cfg. table
    rc=BeeIoTParseCfg((beeiot_cfg_t*) msg);  // lets parse received message as Switch CFG request
		break;

	case CMD_LOGSTATUS: // BeeIoT node sensor data set received
    BHLOG(LOGLORAW) Serial.printf("  BeeIoTParse[%i]: cmd= LOGSTATUS -> should be ACK => ignored\n", msg->hd.index); 
    // intentionally do nothing on node side
    rc=CMD_LOGSTATUS;
		break;

	case CMD_GETSDLOG:
    BHLOG(LOGLORAW) Serial.printf("  BeeIoTParse[%i]: SDLOG Save command: not supported yet\n", msg->hd.index); 
		// for test purpose: dump payload in hex format
			// ToDo: read out SD card and send it package wise to GW
		rc= CMD_GETSDLOG;                      // not supported yet
		break;

	case CMD_RETRY:
    BHLOG(LOGLORAW) Serial.printf("  BeeIoTParse[%i]: RETRY-Request: send same message again\n", msg->hd.index); 
    if(MyMsg.pkg->hd.index == msg->hd.index){    // do we talk about the same Node-TX message we are waiting for ?
      BeeIoTStatus = BIOT_TX;             // Retry Msg -> in TX mode again
      while(!sendMessage(&MyTXData, 0));   // No, send same pkg /w same msgid in sync mode again
      MyMsg.retries++;                     // remember # of retries
      rc=CMD_RETRY;                        // -> wait for ACK by upper layer required
    }else{
      BHLOG(LOGLORAW) Serial.printf("  BeeIoTParse: RETRY but new MsgID[%i] does not match to TXpkg[%i] ->ignored\n",
             msg->hd.index, MyMsg.pkg->hd.index);
      rc = -1;  // retry requested but not for last sent message -> obsolete request
    }
		break;

	case CMD_ACK:   // we should not reach this point -> covered by ISR
    BHLOG(LOGLORAW) Serial.printf("  BeeIoTParse[%i]: ACK received -> Should have been done by ISR ?!\n", msg->hd.index); 
    //  Already done by onReceive() implicitely
      // BHLOG(LOGLORAW) Serial.printf("  BeeIoTParse[%i]: ACK received -> Data transfer done.\n", msg->index); 
      if(MyMsg.pkg->hd.index == msg->hd.index){    // save link to User payload data
        MyMsg.ack = 1;                        // confirm Acknowledge flag
      }
    rc = CMD_ACK;   // Message sending got acknowledged
		break;

	default:
    BHLOG(LOGLORAW) Serial.printf("  BeeIoTParse[%i]: unknown CMD(%d)\n", msg->hd.index, msg->hd.cmd); 
		// for test purpose: dump paload in hex format
    BHLOG(LOGLORAW) hexdump((unsigned char*) msg, BIoT_HDRLEN + msg->hd.frmlen + BIoT_MICLEN);		
		rc= -5;
		break;
	}
	
	return(rc);
}

//******************************************************************************
// BeeIoTParseCfg() 
// Analyze received BeeIoT Message for action
// Input:
//  Msg   Pointer to next BeeIoT Message at MyRXData[RXPkgSrvIdx] for parsing
// Return: rc
//	CMD_CONFIG	    CONFIG Set validated and modem (re)configured
//  -1 CONFIG set failed
//******************************************************************************
int BeeIoTParseCfg(beeiot_cfg_t * pcfg){
int rc;
    if(MyMsg.pkg->hd.index != pcfg->hd.index){    // do we talk about the same Node-TX message we are waiting for ?
      return(-1);
    }
    // parse CFG struct here and configure modem

    rc=CMD_CONFIG;
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

//  configLoraModem();  // setup channel for next send action

// start package creation:
  if(!LoRa.beginPacket(0))            // reset FIFO ptr.; in explicit header Mode
    return(0);                         // still transmitting -> have to come back later

  Serial.flush(); // send all Dbg msg up to here...

  // now lets fill up FIFO:
  LoRa.write(TXData->hd.destID);          // add destination address
  LoRa.write(TXData->hd.sendID);          // add sender address
  LoRa.write(TXData->hd.index);           // add message ID = package serial number
  LoRa.write(TXData->hd.cmd);             // store function/command for GW 
  LoRa.write(TXData->hd.frmlen);          // user-payload = App Frame length
  LoRa.print(TXData->data);            // add AppFrame incl. MIC
  
  // Finish packet and send it in LoRa Mode
  // =0: sync mode: endPacket returns when TX_DONE Flag was achieved
  // =1 async: polling via LoRa.isTransmitting() needed; -> remove LoRa.idle()
  LoRa.endPacket(async);

  BHLOG(LOGLORAR) Serial.printf("  LoRaSend(0x%x>0x%x)[%i](cmd=%d)%s - %iBy\n", 
          TXData->hd.sendID, TXData->hd.destID, TXData->hd.index, TXData->hd.cmd, 
          TXData->data, TXData->hd.frmlen+BIoT_HDRLEN+BIoT_MICLEN);
  BHLOG(LOGLORAR) hexdump((unsigned char*) TXData, BIoT_HDRLEN + TXData->hd.frmlen + BIoT_MICLEN);
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
  if (packetSize < BIoT_HDRLEN || packetSize > MAX_PAYLOAD_LENGTH) {
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
  BHLOG(LOGLORAR) hexdump((unsigned char*) msg, BIoT_HDRLEN);
	
// now check real data:
// start the gate keeper of valid BeeIoT Package data:
	if(msg->hd.sendID != mygw){
		Serial.printf("onReceive: Unknown NodeID detected (0x%02X) -> package rejected\n", (unsigned char)msg->hd.sendID);		
		return;
	}
	if(msg->hd.destID != localAddress){
		Serial.printf("onReceive: Wrong Target GWID detected (0x%02X) -> package rejected\n", (unsigned char)msg->hd.destID);		
		BHLOG(LOGLORAR) Serial.println();
		return;
	}
  // Expected pkg format: Header<NODEID + GWID + index + cmd + frmlen> + AppFrame + MIC
	if(BIoT_HDRLEN+msg->hd.frmlen+BIoT_MICLEN != packetSize){	
		Serial.printf("onReceive: Wrong payload size detected (%i) -> package rejected\n", packetSize);
	  BHLOG(LOGLORAR) Serial.println();
		return;
	}

  // At this point we have a new header-checked BeeIoT Package
  // some debug output: assumed all is o.k.
	BHLOG(LOGLORAW) Serial.printf("\nonReceive: RX(0x%02X>0x%02X)", (unsigned char)msg->hd.sendID, (unsigned char)msg->hd.destID);
	BHLOG(LOGLORAW) Serial.printf("[%i]:(cmd=%i: %s) ", (unsigned char) msg->hd.index, (unsigned char) msg->hd.cmd, beeiot_ActString[msg->hd.cmd]);
  BHLOG(LOGLORAW) Serial.printf("DataLen=%i Bytes\n", msg->hd.frmlen);

  // if it is CMD_ACK pkg -> lets commit to waiting TX-Msg
  if(msg->hd.cmd == CMD_ACK){
    if(msg->hd.index == MyMsg.idx){  // is last TX task waiting for this ACK ?
      MyMsg.ack = 1;              // ACK flag hopefully still polled by waiting TX service
    }else{                        // noone is waiting for this ACK -> ignore it, but leave a note
      BHLOG(LOGLORAR) Serial.printf("onReceive: ACK received -> with no matching msg.index: %i\n", msg->hd.index);
    } // skip RX Queue handling for ACK pkgs.

  }else{  // for any other CMD of RX package -> lets do it on userland side
    if(msg->hd.cmd == CMD_RETRY){      // keep RETRY as ACK to satisfy ACK wait loop at LoRaLog()
      if(msg->hd.index == MyMsg.idx){  // is the last TX task waiting for an ACK  but got RETRY now ?
        MyMsg.ack = 1;              // fake an ACK flag hopefully still polled by waiting TX service
      }else{                        // noone is waiting for an ACK -> but we will process it anyway
        BHLOG(LOGLORAR) Serial.printf("onReceive: RETRY received -> with no matching msg.index: %i\n", msg->hd.index);
      } // nothing done for RETRY here, but parse routine will do the rest---
    }
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