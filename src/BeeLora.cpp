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
#include <stdint.h>
#include <string.h>
#include <time.h>
#include "RTClib.h"
#include "soc/rtc_wdt.h"

// #include <board.h>
// #include <BIoTaes.h>

#include <LoRa.h>         // from LoRa Sandeep Driver
#define BEEIOT_ACTSTRINGS
#define CHANNELTAB_EXPORT // enable channelcfg tab instance here: channeltable_t	txchntab[MAX_CHANNELS]
#include "BeeIoTWan.h"
#undef BEEIOT_ACTSTRINGS
#undef CHANNELTAB_EXPORT  // block double instanciation

#include "beelora.h"      // BeeIoT Lora Message definitions
#include "beeiot.h"       // provides all GPIO PIN configurations of all sensor Ports !
#include "BIotCrypto.h"   // MIC generation and En-/Decryption of Msg payload

//***********************************************
// Global Sensor Data Objects
//***********************************************
// Central Database of all measured values and runtime parameters
extern dataset		bhdb;		// central node status DB -> main.cpp
extern unsigned int	lflags;		// BeeIoT log flag field (masks see beeiot.h)
int     islora=0;				// =1: we have an active LoRa Node

// GPIO PINS of current connected LoRa Modem chip (SCK, MISO & MOSI are system default)
const int csPin     = LoRa_CS;	// LoRa radio chip select
const int resetPin  = LoRa_RST;	// LoRa radio reset not used
const int irqPin    = LoRa_DIO0;// change for your board; must be a hardware interrupt pin

// Lora Modem default configuration
RTC_DATA_ATTR struct LoRaRadioCfg_t{
    // addresses of GW and node used for package identification -> might get updated by JOIN_CONFIG acknolewdge pkg
    byte nodeid	    = NODEIDBASE;	// My address/ID (of this node/device) -> Preset with JOIN defaults
    byte gwid		= GWIDx;		// My current gateway ID to talk with  -> Preset with JOIN defaults
    byte chcfgid	= 0;			// channel cfg ID of initialzed ChannelTab[]-config set: default id=0 => EU868_F1

    // following cfg parameters are redefined by a new channel-cfg set reflected by the channel ID provided via CONFIG CMD
    long freq	= FREQ;			// =EU868_F1..9,DN (EU868_F1: 868.1MHz)
    s1_t pw		= TXPOWER;		// =2-16  TX PA Mode (14)
    sf_t sf		= SPREADING;	// =0..8 Spreading factor FSK,7..12,SFrFu (1:SF7)
    bw_t bw		= SIGNALBW;		// =0..3 RFU Bandwidth 125-500 (0:125kHz)
    cr_t cr		= CODING;		// =0..3 Coding mode 4/5..4/8 (0:4/5)
    int	 ih		= IHDMODE;		// =1 implicite Header Mode (0)
    u1_t ihlen	= IHEADERLEN;	// =0..n if IH -> header length (0)
    u1_t nocrc	= NOCRC;		// =0/1 no CRC check used for Pkg (0)
    u1_t noRXIQinversion = NORXIQINV;	// =0/1 flag to switch RX+TX IQinv. on/off (1)
    // runtime parameters for messages
    int	msgCount    = 0;			// gobal serial counter of outgoing messages 0..255 round robin
    int	joinCount   = 0;			// global JOIN packet counter
    int	joinRetryCount= 0;		// # of JOIN+REJOIN Request Actions
} LoRaCfg;

RTC_DATA_ATTR byte BeeIoTStatus;	// Current Status of BeeIoT WAN protocol (not OPMode !) -> beelora.h
extern void setRTCtime(uint8_t yearoff, uint8_t month, uint8_t day,  uint8_t hour, uint8_t min, uint8_t sec);

extern int report_interval; // interval between BIoT Reports

//////////////////////////////////////////////////
// CONFIGURATION (FOR APPLICATION CALLBACKS BELOW)
//////////////////////////////////////////////////
// all variables in RTC Mem for sleep mode persistence

// Node unique device ID (LSBF) -> will be initialized with local BoardID in Setup_Lora()
RTC_DATA_ATTR  u1_t DEVEUI[8]	= { 0xCC, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08 };

// BIoT application service ID (LSBF) -> constant in (see BeeIoTWAN.h)
RTC_DATA_ATTR  u1_t JOINEUI[8]	= {BIoT_EUID};   // aka AppEUI

// Node-specific AES key (derived from device EUI)
RTC_DATA_ATTR  u1_t DEVKEY[16]	= { 0xBB, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F};

// BIoT Nws & App service Keys for pkt and frame encryption
RTC_DATA_ATTR  u1_t AppSKey[16]	= { 0xAA, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F};
RTC_DATA_ATTR  u1_t NwSKey[16]	= { 0xDD, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F};


#define MAXRXPKG		3		// Max. number of parallel processed RX packages
RTC_DATA_ATTR  byte BeeIotRXFlag =0;		// Semaphor for received message(s) 0 ... MAXRXPKG-1
RTC_DATA_ATTR  byte RXPkgIsrIdx  =0;		// index on next LoRa Package for ISR callback Write
RTC_DATA_ATTR  byte RXPkgSrvIdx  =0;		// index on next LoRa Package for Service Routine Read/serve
beeiotmsg_t MyMsg;				// Lora message on the air (if MyMsg.data != NULL)
beeiotpkg_t MyRXData[MAXRXPKG];	// received message for userland processing
beeiotpkg_t MyTXData;			// Lora package buffer for sending

// define BeeIoT LoRA Status strings
// for enum def. -> see beelora.h
const char * beeiot_StatusString[] = {
	[BIOT_NONE]   = "NoLORA",
	[BIOT_JOIN]   = "JOIN",
	[BIOT_REJOIN] = "REJOIN",
	[BIOT_IDLE]	  = "IDLE",
	[BIOT_TX]	  = "TX",
	[BIOT_RX]	  = "RX",
	[BIOT_SLEEP]  = "SLEEP",
	[BIOT_DEEPSLEEP]  = "DEEPSLEEP",
};
extern  RTC_DS3231 rtc; // global RTC instance of node

//********************************************************************
// Function Prototypes
//********************************************************************
void configLoraModem(LoRaRadioCfg_t * cfg);
int	 BeeIoTJoin		(byte Joinstatus);
int  BeeIoTWakeUp	(void);
void BeeIoTSleep	(void);
int  BeeIoTParse	(beeiotpkg_t * msg);
int  BeeIoTParseCfg(beeiot_cfg_t * pcfg);
int  SetChannelCfg	(byte  channelidx);
int  sendMessage	(beeiotpkg_t * TXData, int sync);
void onReceive		(int packetSize);
int  WaitForAck		(byte *BIOTStatus, beeiotmsg_t *Msg, beeiotpkg_t * Pkg, LoRaClass *Modem);
void BIoT_getmic	(beeiotpkg_t * pkg, int dir, byte * mic);
extern void LoRaMacJoinComputeMic( const uint8_t *buffer, uint16_t size, const uint8_t *key, uint32_t *mic );
// in main()
extern void ResetNode(uint8_t p1, uint8_t p2, uint8_t p3);
void hexdump(unsigned char * msg, int len);

// in sdcard.cpp
extern int sd_get_dir(uint8_t p1, uint8_t p2, uint8_t p3, char * dirline);


//*********************************************************************
// Setup_LoRa(): init BEE-client object: LoRa
//*********************************************************************
int setup_LoRa(int reentry) {
	islora = 0;;

#ifdef LORA_CONFIG
	if(!reentry){	// after PON Reset only
		BHLOG(LOGLORAR) Serial.printf("  LoRa: Cfg Lora Modem for BIoTWAN v%d.%d (on %ld Mhz)\n",
			(int)BIoT_VMAJOR, (int)BIoT_VMINOR, LoRaCfg.freq);
		BeeIoTStatus = BIOT_NONE;
	}else{
		BHLOG(LOGLORAR) Serial.printf("  LoRa: WakeUpMode:%i in Status: %s\n", reentry, beeiot_StatusString[BeeIoTStatus]);
  }

	// Initial Modem & channel setup:
	// Set GPIO(SPI) Pins, Reset device, Discover SX1276 (only by Lora.h lib) device, results in:
	// OM=>STDBY/Idle, AGC=On(0x04), freq = FREQ, TX/RX-Base = 0x00, LNA = BOOST(0x03), TXPwr=17
	if (!LoRa.begin(LoRaCfg.freq)) {          // initialize ratio at LORA_FREQ [MHz]
		// for SX1276 only (Version = 0x12)
		Serial.println("  LoRa: SX1276 not detected. Check your GPIO connections.");
		return(islora);                          // No SX1276 module found -> Stop LoRa protocol
	}
	// For debugging: Stream Lora-Regs:
	//   BHLOG(LOGLORAR) LoRa.dumpRegisters(Serial); // dump all SX1276 registers in 0xyy format for debugging

	// Create a unique Node DEVEUI from BoardID(only Byte 0-6 can be used) > 0xbbbbbbfffebbbbbb
	// max length: LENDEVEUI !
		byte * pb = (byte*) & bhdb.BoardID; // get BytePtr for field handling
		DEVEUI[0] = (byte) pb[5];           // get byte stream from back to start
		DEVEUI[1] = (byte) pb[4];
		DEVEUI[2] = (byte) pb[3];
		DEVEUI[3] = 0xFF;   // fill up to 8 By.
		DEVEUI[4] = 0xFE;   // fill up to 8 By.
		DEVEUI[5] = (byte) pb[2];
		DEVEUI[6] = (byte) pb[1];
		DEVEUI[7] = (byte) pb[0];

	if(!reentry){	// after PON Reset only
		LoRaCfg.joinRetryCount =0;	// No JOIN yet
		LoRaCfg.msgCount       =0;  // reset pkg counter to default
		// BeeIoT-Wan Presets to default LoRa channel (if apart from the class default)
		SetChannelCfg(JOINCFGIDX);      // initialize LoraCfg field by default modem cfg for joining
	} else {
  	// BeeIoT-Wan Presets to default LoRa channel (if apart from the class default)
		SetChannelCfg(LoRaCfg.chcfgid);      // initialize LoraCfg field by assigned modem cfg
  }
	configLoraModem(&LoRaCfg);

	// lets see where we are...
	//  BHLOG(LOGLORAR)  LoRa.dumpRegisters(Serial); // dump all SX1276 registers in 0xyy format for debugging

	// Setup RX Queue management
	BeeIotRXFlag= 0;              // reset Semaphor for received message(s) -> polled by Sendmessage()
	RXPkgSrvIdx = 0;              // preset RX Queue Read Pointer
	RXPkgIsrIdx = 0;              // preset RX Queue Write Pointer

	// Reset TX Msg buffer
	MyMsg.idx=0;
	MyMsg.ack=0;
	MyMsg.retries=0;
	MyMsg.pkg  = (beeiotpkg_t*) NULL;

	// Assign IRQ callback on DIO0
	LoRa.onReceive(onReceive);    // (called by loRa.handleDio0Rise(pkglen))
	BHLOG(LOGLORAR) Serial.printf("  LoRa: assign ISR to DIO0  - default: GWID:0x%02X, NodeID:0x%02X, Channel:%i\n", LoRaCfg.gwid, LoRaCfg.nodeid, LoRaCfg.chcfgid);
	islora=1;                     // Declare:  LORA Modem is basically active now, but not joined yet!

	if(!reentry){	// AFter (Pwr-) Reset only
		// From now on : Try JOIN to a GW
		BeeIoTStatus = BIOT_JOIN; // have to join to a GW
    	LoRa.sleep(); // stop modem and clear FIFO
		BHLOG(LOGLORAW)  Serial.printf("  Lora: remain in JOIN Mode\n");
	}
#endif

  return(islora);
} // end of Setup_LoRa()


//***********************************************************************
// Lora Modem Init
// - Can also be used for each channel hopping
// - Modem remains in Idle (standby) mode
//***********************************************************************
void configLoraModem(LoRaRadioCfg_t * pcfg) {
  long sbw = 0;
  byte coding=0;

  BHLOG(LOGLORAR) Serial.printf("  LoRaCfg: Set Modem/Channel-Cfg[%i]: %ldMhz, SF=%i, TXPwr:%i",
        pcfg->chcfgid, pcfg->freq, 6+(byte)pcfg->sf, (byte)pcfg->pw);
  LoRa.sleep(); // stop modem and clear FIFO
  // set frequency
  LoRa.setFrequency(pcfg->freq);

  // RFOUT_pin could be RF_PACONFIG_PASELECT_PABOOST or RF_PACONFIG_PASELECT_RFO
  //   - RF_PACONFIG_PASELECT_PABOOST -- LoRa single output via PABOOST, maximum output 20dBm
  //   - RF_PACONFIG_PASELECT_RFO     -- LoRa single output via RFO_HF / RFO_LF, maximum output 14dBm
  LoRa.setTxPower((byte)pcfg->pw, PA_OUTPUT_PA_BOOST_PIN); // no boost mode, outputPin = default

  // SF6: REG_DETECTION_OPTIMIZE = 0xc5
  //      REG_DETECTION_THRESHOLD= 0x0C

  // SF>6:REG_DETECTION_OPTIMIZE = 0xc3
  //      REG_DETECTION_THRESHOLD= 0x0A
  // implicite setLdoFlag();
  LoRa.setSpreadingFactor(6 + (byte)pcfg->sf);   // ranges from 6-12,default 7 see API docs

  switch (pcfg->bw) {
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
			      Serial.printf("\n    configLoraModem:  Warning_wrong bw setting: %i\n", pcfg->bw);
  }
  LoRa.setSignalBandwidth(sbw);    // set Signal Bandwidth
  BHLOG(LOGLORAR) Serial.printf(", BW:%ld", sbw);

  switch( pcfg->cr ) {
        case CR_4_5: coding = 5; break;
        case CR_4_6: coding = 6; break;
        case CR_4_7: coding = 7; break;
        case CR_4_8: coding = 8; break;
		    default:    //  ASSERT(0);
			      Serial.printf("\n    configLoraModem:  Warning_wrong cr setting: %i\n", pcfg->cr);
        }
  LoRa.setCodingRate4((byte)coding);
  BHLOG(LOGLORAR) Serial.printf(", CR:%i", (byte)coding);

  LoRa.setPreambleLength(LPREAMBLE);    // def: 8Byte
  BHLOG(LOGLORAR) Serial.printf(", LPreamble:%i\n", LPREAMBLE);
  LoRa.setSyncWord(LoRaSW);          // ranges from 0-0xFF, default 0x34, see API docs
  BHLOG(LOGLORAR) Serial.printf("    SW:0x%02X", LoRaSW);

  if(pcfg->nocrc){
    BHLOG(LOGLORAR) Serial.print(", NoCRC");
    LoRa.noCrc();                       // disable CRC
  }else{
    BHLOG(LOGLORAR) Serial.print(", CRC");
    LoRa.crc();                         // enable CRC
  }

  if(pcfg->noRXIQinversion){          // need RX with Invert IQ
    BHLOG(LOGLORAR) Serial.print(", noInvIQ");
    LoRa.disableInvertIQ();             // REG_INVERTIQ  = 0x27, REG_INVERTIQ2 = 0x1D
  }else{
    BHLOG(LOGLORAR) Serial.print(", InvIQ\n");
    LoRa.enableInvertIQ();              // REG_INVERTIQ(0x33)  = 0x66, REG_INVERTIQ2(0x3B) = 0x19
  }

  if(pcfg->ih){
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
//   0:  no joined GW found  -> BeeIoTStatus still BIOT_JOIN
//  -1:  JOIN failed -> TO or wrong channel cfg.
//  -2:  Max. # of retries
//****************************************************************************************
int BeeIoTJoin(byte joinstatus){
beeiot_join_t * pjoin;  // ptr. on JOIN message format field (reusing global MyTXData buffer)
int i;
int rc;
int cmd=0;	// return from beeiotparse()

pjoin = (beeiot_join_t *) & MyTXData; // fetch global Msg buffer

 // Prepare Pkg Header
  if(joinstatus == BIOT_JOIN){
    pjoin->hd.cmd    = CMD_JOIN;    // Lets Join
    pjoin->hd.sendID = NODEIDBASE;  // that's me by now but finally not checked in JOIN session
    LoRaCfg.nodeid   = NODEIDBASE;  // store "JOIN"-NodeID for onreceive() check
    pjoin->hd.pkgid  = 0xFF;        // ser. number of JOIN package; well, could be any ID 0..0xFF
    BHLOG(LOGLORAW) Serial.printf("  BeeIoTJoin: Start searching for a GW\n");
  }else{ // REJOIN of a given status
    pjoin->hd.cmd    = CMD_REJOIN;  // Lets Join again
    pjoin->hd.sendID = LoRaCfg.nodeid;   // use GW of well known last node id for reactivation
    pjoin->hd.pkgid  = LoRaCfg.msgCount; // ser. number of JOIN package; for rejoining use std. counter
    BHLOG(LOGLORAW) Serial.printf("  BeeIoTJoin: Connection lost - Re-Joining with default ChannelCfg\n");
  }
  pjoin->hd.destID = GWIDx;         // We start joining always with the default BeeIoT-WAN GWID
  pjoin->hd.frmlen = (uint16_t)sizeof(joinpar_t); // length of JOIN payload: ->info

  // setup join frame ->info
  memcpy((byte*)pjoin->info.joinEUI, (byte*)JOINEUI, LENJOINEUI);
  memcpy((byte*)pjoin->info.devEUI,  (byte*)DEVEUI,  LENDEVEUI);
  pjoin->info.frmid[0] = (byte)LoRaCfg.msgCount >> 8;   // copy MSB of msgcount
  pjoin->info.frmid[1] = (byte)LoRaCfg.msgCount & 0xFF;  // copy LSB
  pjoin->info.vmajor = (byte)BIoT_VMAJOR;
  pjoin->info.vminor = (byte)BIoT_VMINOR;

  BIoT_getmic( & MyTXData, UP_LINK,(byte*) pjoin->mic); // MIC generation of (pjoin-hdr + pjoin-info)

  // remember new Msg (for possible RETRIES)
  // have to do it before sendmessage -> is used by ISR routine in case of (fast) ACK response
  MyMsg.ack = 0;                    // no ACK expected, but a CONFIG pkg
  MyMsg.idx = pjoin->hd.pkgid;      // set index reference for matching CONFIG pkg
  MyMsg.retries = 0;
  MyMsg.pkg = (beeiotpkg_t*) & MyTXData;  // save pkg rootptr

  SetChannelCfg(JOINCFGIDX);      // initialize LoraCfg field by default modem cfg for RE/JOIN
  configLoraModem(&LoRaCfg);      // set modem to default JOIN settings

  BeeIoTStatus = BIOT_JOIN;          // start TX_JOIN session
  while(!sendMessage(&MyTXData, 0)); // send it in sync mode

  // Lets wait for expected CONFIG Msg
  BeeIotRXFlag=0;             // spawn ISR RX flag
  RXPkgSrvIdx = RXPkgIsrIdx;  // declare RXQueue: RD/WR index on same location -> empty
  // Activate RX Contiguous flow control
  do{
    MyMsg.ack=0;                // reset ACK & Retry flags
    LoRa.receive(0);            // Activate: RX-Continuous in expl. Header mode

    i=0;  // reset wait counter
    BHLOG(LOGLORAW) Serial.printf("  BeeIoTJoin: waiting for RX-CONFIG Pkg. in RXCont mode:");
    while (!BeeIotRXFlag && (i<MAXRXACKWAIT*2)){  // till RX pkg arrived or max. wait time is over
        BHLOG(LOGLORAW) Serial.print("o");
        mydelay2(500,100);            // count wait time in msec. -> frequency to check RXQueue
        i++;
    }
    // notice # of JOIN Retries
    if(LoRaCfg.joinRetryCount++ == 10){ // max. # of acceptable JOIN Requests reached ?
// 30.12.2021: keep report_interval of initial 1 minute -> otherwise retry counter increase too much.
//      report_interval *= 2;    		// wait 10-times longer to try again for saving power
      BHLOG(LOGLORAW) Serial.printf("\n  BeeIoTJoin: After %i retries: Reduce Retry frequency to every %i sec.",
            LoRaCfg.joinRetryCount, report_interval);
      LoRaCfg.joinRetryCount = 0;
    };        // report_interval will be reinitialized by CONFIG with successfull JOIN request

    rc=0;	// default: no GW in range
    if(i>=MAXRXACKWAIT*2){       // TO condition reached, no RX pkg received=> no GW in range ?
        BHLOG(LOGLORAW) Serial.println(" None.");
        // RX Queue should still be empty: validate it:
        if(!BeeIotRXFlag & (RXPkgSrvIdx != RXPkgIsrIdx)){ // RX Queue validation check: realy empty ?
          Serial.printf("  BeeIotJoin: This case should never happen: Queue-RD(%i)/WR(%i) Index different when BeeIoTRXFlag==0 -> fixed\n",RXPkgSrvIdx, RXPkgIsrIdx);
          // ToDo: any correcting action ? or exit ?
          RXPkgSrvIdx = RXPkgIsrIdx;  // no Queue entry: RD & WR ptr. must be identical
        }
		rc = 0;				// No GW found
        cmd=CMD_RETRY;      // initiate JOIN request again
    }else{  // RX pkg received
      // ISR has checked Header and copied pkg into MyRXData[RXPkgSrvIdx] already -> now parse it
      BHLOG(LOGLORAR) Serial.printf("  BeeIotJoin: RX pkg received: %s\n", beeiot_ActString[MyRXData[RXPkgSrvIdx].hd.cmd]);
      BHLOG(LOGLORAR) Serial.printf("  BeeIotJoin: RX Queue Status: SrvIdx:%i, IsrIdx:%i, new PkgID:%i, RXFlag:%i\n",
              RXPkgSrvIdx, RXPkgIsrIdx, MyRXData[RXPkgSrvIdx].hd.pkgid, BeeIotRXFlag);

      cmd = BeeIoTParse(&MyRXData[RXPkgSrvIdx]); // parse MyRXData[RXPkgSrvIdx] for action
      if(cmd < 0){
        BHLOG(LOGLORAW) Serial.printf("  BeeIotJoin: parsing of Msg[%i]failed rc=%i\n", MyRXData[RXPkgSrvIdx].hd.pkgid, cmd);
        // Detailed error analysis/message done by BeeIoTParse() already
        cmd=CMD_RETRY;
		rc = -1;
      }
      // release RX Queue buffer
      if(++RXPkgSrvIdx >= MAXRXPKG){  // no next free RX buffer left in sequential queue
        BHLOG(LOGLORAR) Serial.printf("  BeeIotJoin: RX Buffer end reached, switch back to buffer[0]\n");
        RXPkgSrvIdx=0;  // wrap around
      }
      BeeIotRXFlag--;   // and we can release one more RX Queue buffer
      if((!BeeIotRXFlag) & (RXPkgSrvIdx != RXPkgIsrIdx)){ // RX Queue validation check: realy empty ?
        Serial.printf("  BeeIotJoin: This case should never happen 2: Queue-RD(%i)/WR(%i) Index different when BeeIoTRXFlag==0 -> fixed\n",RXPkgSrvIdx, RXPkgIsrIdx);
          // ToDo: any correcting action ? or exit ?
          RXPkgSrvIdx = RXPkgIsrIdx;  // no Queue entry: RD & WR ptr. must be identical
      }
    } // New Pkg parsed

    if( cmd==CMD_RETRY) { // it is requested to send JOIN again (because either no or wrong response or explicitly requested by GW)
       if(MyMsg.retries < MSGMAXRETRY){       // enough Retries ?
          BeeIoTStatus = BIOT_JOIN;           // No , start TX session but in JOIN mode only
          while(!sendMessage(&MyTXData, 0));  // Send same pkg /w same msgid in sync mode again
          MyMsg.retries++;                    // remember # of retries
          BHLOG(LOGLORAW) Serial.printf("  BeeIotJoin: JOIN-Retry: #%i (Overall retries: %i)\n", MyMsg.retries, LoRaCfg.joinRetryCount);
          i=0;  // reset ACK wait loop for next try
		  rc=0;
        }else{  // Max. # of Retries reached -> give up
          BHLOG(LOGLORAW) Serial.printf("  BeeIotJoin: JOIN-Request stopped (Retries: #%i)\n", MyMsg.retries);
          // clean up TX Msg buffer (ISR checks for MyMsg.idx == RXData.index only)
          LoRa.sleep(); // stop modem and clear FIFO
          MyMsg.ack     = 0;
          MyMsg.idx     = 0;
          MyMsg.retries = 0;
          MyMsg.pkg     = (beeiotpkg_t *) NULL;  // TX done -> release LoRa package buffer
          BeeIoTStatus  = BIOT_JOIN;             // remain in joining mode
          rc=-2;           // TX-timeout -> max. # of Retries reached
          return(rc);       // give up and no RX Queue check needed neither -> GW dead ?
        } // if(Max-Retry)
    } // Retry action

    if(cmd==CMD_CONFIG){      // we are joined with a GW
		configLoraModem(&LoRaCfg);      // set modem to new channel settings
      	BeeIoTStatus = BIOT_IDLE;		// we are connected
	  	LoRaCfg.joinRetryCount =0;	// connection is up -> reset fail counter
      	rc=1; 						// leave this loop: positive RC
    }
  } while(cmd == CMD_RETRY); // Retry JOINing again

  if(BeeIoTStatus==BIOT_IDLE){
    BHLOG(LOGLORAW) Serial.printf("  Lora: Joined! New: GWID:0x%02X, NodeID:0x%02X, msgcount:%d\n",
        LoRaCfg.gwid, LoRaCfg.nodeid, LoRaCfg.msgCount);
    BHLOG(LOGLORAW) Printhex( DEVEUI,  8, "    DEVEUI: 0x-");
    BHLOG(LOGLORAW) Serial.println();
    BHLOG(LOGLORAW) Printhex( JOINEUI, 8, "   JOINEUI: 0x-");
    BHLOG(LOGLORAW) Serial.println();   // == APPEUI
    BHLOG(LOGLORAR) Printhex( DEVKEY, 16, "    DEVKEY: 0x-", 2);
    BHLOG(LOGLORAR) Serial.println();
//    BHLOG(LOGLORAR) Printhex( (u1_t*) & bhdb.BoardID, 6, "   BoardID: 0x-", 6, 1); BHLOG(LOGLORAR)Serial.print(" -> ");
//    BHLOG(LOGLORAR) Printbit( (u1_t*) & bhdb.BoardID, 6*8, "  0b-", 1, 1); BHLOG(LOGLORAR)Serial.println();

    LoRaCfg.msgCount++; // define Msgcount for next pkg (based by CONFIG Data)
    LoRa.idle();
    rc=1;
  }
 return(rc);
}

//***********************************************************************
// BIoT_getmic()
// Create pkg integrity code for pkg->hdr + pkg->data (except MIC itself)
// INPUT:
//  pkg   ptr on MSG-Buffer (HDR + payload)
//  dir   UP_LINK(Clinet->GW), DOWN_LINK (GW->Client)
// OUTPUT:
//  mic   ptr. on Msg Integrity code (4 Byte field)
//***********************************************************************
void BIoT_getmic(beeiotpkg_t * pkg, int dir, byte * mic) {
uint32_t bufmic = 0x11223344;
uint32_t* pduid;

  // ToDo get MIC calculation:
  // byte cmac[16]={0x11,0x22,0x33,0x44};
  // cmac = aes128_cmac(NwkKey, MHDR | JoinEUI | DevEUI | DevNonce)
  if(pkg->hd.cmd == CMD_JOIN || pkg->hd.cmd == CMD_REJOIN){
    // For JoinPkg:
    LoRaMacJoinComputeMic( (const uint8_t*) pkg, (uint16_t) pkg->hd.frmlen+BIoT_HDRLEN,
               (const uint8_t*) &NwSKey, (uint32_t*) &bufmic );
  }else{
    // For Upload Pkg:
	  pduid = (uint32_t*) &DEVEUI;
	  BHLOG(LOGLORAR) printf("  JS_ValidateMic: duid[%i] = 0x%X\n", pkg->hd.pkgid, *pduid);
    LoRaMacComputeMic( (const uint8_t*)pkg, (uint16_t) pkg->hd.frmlen+BIoT_HDRLEN, (const uint8_t*) &NwSKey,
             (const uint32_t) *pduid, dir, (uint32_t) pkg->hd.pkgid, (uint32_t*) &bufmic );
  }

  mic[0] = (bufmic >>24) & 0xFF;
  mic[1] = (bufmic >>16) & 0xFF;
  mic[2] = (bufmic >> 8) & 0xFF;
  mic[3] = (bufmic) & 0xFF;

  BHLOG(LOGLORAR) Serial.printf("  BIoT_getmic: (0x%X) -> Add MIC[4] = %02X %02X %02X %02X for Msg[%d]\n",
      bufmic, mic[0], mic[1], mic[2], mic[3], pkg->hd.pkgid);
  return;
}


//***********************************************************************
// BeeIoTWakeUp()
//***********************************************************************
int BeeIoTWakeUp(void){
  BHLOG(LOGLORAR) Serial.printf("  BeeIoTWakeUp: Enter Idle Mode\n");
  // toDo. kind of Setup_Lora here
  configLoraModem(&LoRaCfg);
  BeeIoTStatus = BIOT_IDLE;     // modem is active
  return(islora);
}

//***********************************************************************
// BeeIoTSleep()
//***********************************************************************
void BeeIoTSleep(void){
    BHLOG(LOGLORAR) Serial.printf("  BeeIoTSleep: Enter Sleep Mode\n");
    BeeIoTStatus = BIOT_SLEEP;  // go into power safe mode
    LoRa.sleep(); // Lora.sleep() + spi.end()
  }

//****************************************************************************************
// LoraLog()
// Converts UserLog-String into LoRa Message format
// Controls BIoTWAN flow control: Send -> Wait for Ack ... and in case send retries:
// 1. Check of valid LoRa Modem Send Status: BIOT_IDLE (if not try to recover status)
// 2. Prepare TX package with LogStatus data
// 3. Store TX Session data for ISR and retry loops
// 4. Initiate TX session
// 5. Bypass FlowControl in NoAck Mode
// 6. Start of ACK wait loop
//    7. Activate flow control: start RX Contiguous Mode
//      8. Check for ACK Wait Timeout
//        9. No ACK received -> initiate a retry loop
//        10. Max. # of Retries reached -> give up (-99)
//            -> Goto Sleep Mode and set REJOIN status
// 11. Start RX1 Window: TX session Done (ACK received) Give GW the option for another Job
//  12. Check for ACK Wait Timeout -> yes, but no RX1 job expected => Done (0)
// 13. Either RX1 job received or RETRY / REJOIN as Ack from previous TX session
//    14. BIoTParse() RX-Queue pkg: ISR has checked Header and copied pkg into MyRXData[RXPkgSrvIdx]
//        -> CMD_RETRY:  TX pkg will be sent again -> RX1 loop will wait for ACK
//        -> CMD_REJOIN: BIoTStatus = JOIN  -> RX1 loop ends
//        -> new RX1 CMD: will be processed as usual (ACK wait loop)
// 15. Process (RE-)JOIN Request from GW
//
// INPUT:
//  outgoing  ptr on Log data string
//  outlen    length of data string ( could be less then stringlen())
//  noack     =0 ack (incl Retries) requested; =1 no ACK check needed
//
// OUTPUT: (used global/public)
//  BeeIoTStatus  Lora session status field (-> beelora.h) (RTC resid.)
//  LoRaCfg       Lora Modem Channel cfg data (RTC resid.)
//  MyMsg         TX session data root (API to ISR; cleared after TX job)
//  MyTXData      TX Message buffer prepared for sending
//  MyRXData      RX Queue for incoming messages (filled by ISR)
//  BeeIotRXFlag  RX Queue level Semaphor >0 == Queue has (a) message(s)
//  RXPkgSrvIdx   Index to next RX Queue buffer for parsing
//  RXPkgISRIdx   Index to next RX Queue buffer for writing by ISR
//
// Return:
//  >0   all return codes from BeeIoTParse()
//  0:  Send LOG successfull & all RX Queue pkgs processed
// -1:  parsing failed -> message dropped
// -95: wrong Input parameters: no data
// -96: no joined GW found  -> forced BeeIoTStatus = BIOT_JOIN
// -97: modem still sleeping; can't wake up now -> retry later
// -98: no compatible LoRa modem detected (SX127x)
// -99: TX timed out: waiting for ACK -> max. # of retries reached
//                    -> forced BeeIoTStatus = BIOT_REJOIN
//****************************************************************************************
int LoRaLog( const uint8_t *outgoing, uint16_t outlen, int noack) {
uint16_t length;  // final length of TX msg payload
int rc;       // generic return code
int ackloop;  // ACK wait loop counter

  if(!outgoing || outlen <= 0)
    return(-95);                  // nothing to send: problem on user side

// 1. Check for a valid LoRa Modem Send Status
  BHLOG(LOGLORAR) Serial.printf("  LoRaLog: BeeIoTStatus = %s\n", beeiot_StatusString[BeeIoTStatus]);
  if(BeeIoTStatus == BIOT_NONE){  // Any LoRa Modem already detected/configured ?
    if(!setup_LoRa(0))
      return(-98);                // no LoRa Modem detected-> no TX message
  }
  if(BeeIoTStatus == BIOT_SLEEP) {// just left (Deep) Sleep mode ?
    if(!BeeIoTWakeUp())           // we have to wakeup first
      return(-97);                // problem to wake up -> try later again
  }
  // Check JOIN Status first
  // Still need a joined GW before to send anything (>JOIN) or GW requests a new JOIN (>REJOIN) Cmd ?
  if((BeeIoTStatus == BIOT_JOIN) || (BeeIoTStatus == BIOT_REJOIN)){
    if(BeeIoTJoin(BeeIoTStatus) <= 0){
    	BHLOG(LOGLORAW)  Serial.printf("  LoraLog: BeeIoT JOIN failed -> skipped LoRa Logging\n");
		// e.g. in case of GW was restarted -> new node registration needed /w Join-Node ID
 		BeeIoTStatus = BIOT_JOIN;	// anyway next must be a JOIN Request (no Rejoin anymore)
    	return(-96);                // by now no gateway in range -> try again later
    }
	mydelay2(1000, 0);		// gracetime for GW to prepare modem on new channel
  }

  // 2. Prepare TX package with LogStatus data
  // a) check/limit xferred string length
  if(outlen > BIoT_FRAMELEN){      // length exceeds frame buffer space ?
    BHLOG(LOGLORAW) Serial.printf("  LoRaLog: data length exceeded, had to cut 0x%02X -> 0x%02X\n", outlen, BIoT_FRAMELEN);
    length = BIoT_FRAMELEN;        // limit payload length incl. EOL(0D0A) to max. framelen
  }else{
    length = outlen;             // get real user-payload length
  }

  // b) Prepare BeeIoT TX package: in MyTXData
	MyTXData.hd.destID = LoRaCfg.gwid;     // remote target GW
	MyTXData.hd.sendID = LoRaCfg.nodeid;   // that's me
	MyTXData.hd.pkgid  = LoRaCfg.msgCount; // serial number of sent packages (sequence checked on GW side!)
	MyTXData.hd.frmlen = (uint16_t) length;   // length of user payload data incl. any EOL - excl. MIC
	memcpy(&MyTXData.data, outgoing, length); // get BIoT-Frame payload data
#ifdef DSENSOR2
	// prepre a binary data stream
	MyTXData.hd.cmd    = CMD_DSENSOR;		// define type of action: BIoTApp session Command for LogStatus Data processing
#else
	// prepare a ascii string
	MyTXData.hd.cmd    = CMD_LOGSTATUS;		// define type of action: BIoTApp session Command for LogStatus Data processing
    MyTXData.data[length-1]=0;				// assure EOL = 0
#endif
	// calculate evaluation code of Msg
	BIoT_getmic( & MyTXData, UP_LINK, (byte*) & MyTXData.data[length]); // MIC CMAC generation of (TX-hdr + TX-payload)

  // 3. remember new Msg (for mutual RETRIES and ACK checks)
  // have to do it before sendmessage -> MyMsg-Pkg is used by ISR for TX-ACK response
	MyMsg.ack     = 0;                  	// in sync mode: checked by sendMessage() and set by ISR
	MyMsg.idx     = LoRaCfg.msgCount;
	MyMsg.retries = 0;                  	// TX retry counter in transmission error case
	MyMsg.pkg     = (beeiotpkg_t*) & MyTXData;  // remember TX Message of session

  // 4. Initiate TX session
  // ToDo: Set up Timer watchdog for TXDone wait and act accordingly
	BeeIoTStatus = BIOT_TX;             // start TX session
	while(!sendMessage(&MyTXData, 0));  // send it in sync mode: wait for RXDone

  // 5. Bypass FlowControl in NoAck Mode
	if(noack){ // bypass flow control -> no wait for (any) ACK requested
		// clean up TX Msg buffer
		MyMsg.pkg     = (beeiotpkg_t *) NULL;
		MyMsg.ack     = 0;
		MyMsg.idx     = 0;
		MyMsg.retries = 0;

		BHLOG(LOGLORAW) Serial.printf("  LoRaLog: Msg(#%i) Done; RX Queue Status SrvIdx:%i, IsrIdx:%i, Length:%i\n",
							LoRaCfg.msgCount, RXPkgSrvIdx, RXPkgIsrIdx, BeeIotRXFlag);
		LoRaCfg.msgCount++;           // increment global sequential TX package/message ID for next session
		BeeIoTSleep();                // prepare Modem for (Deep) Sleep -> save power
									  // ToDo: Reduce PA boost power level
		return(0);                    // Assumed success: No Ack response expected and we skip waiting for further CMDs
	}

  // 6. Start of ACK wait loop:
  // Lets wait for expected ACK Msg & afterwards check RX Queue for some seconds
  rc=0;
  do{  // until BeeIoT Message Queue is empty

	// 7. Activate flow control: RX Continuous in expl. Header mode
	rc = WaitForAck(&BeeIoTStatus, &MyMsg, &MyTXData, &LoRa);
	if(rc < 0){
		return(rc);	// No Ack received even not after amount of retries: give up
		// => JOIN Mode has been entered already
	}

    // We got ACK flag from ISR: But it could be from CMD_ACK, CMD_ACKRX1, CMD_ACKBCN, CMD_RETRY or CMD_JOIN package)
    //    if(CMD_ACK)   -> nothing to do: ISR has left RX Queue empty => no RX1 package by GW expected
    //    if(CMD_ACKRX1) -> nothing to do: ISR has left RX Queue empty => but room for another RX1 package by GW
    //    if(CMD_RETRY || CMD_REJOIN || CMD_ACKBCN) -> ISR has filled up RXQUEUE buffer and incr. BeeIotRXFlag++
    // If RETRY or REJOIN: no RX1 job can be expected from GW
    //    => RX1 loop will
    //        - process current Queue pkg first: 'Ack fall through' (ACK Wait loop is shortcut by BeeIotRXFlag)
    //        - if RETRY: BIoTParse has resend last TX pkg -> RX1 loop will take over ACK wait
    //        - if REJOIN: BIoTParse has just set REJOIN Status -> RX1 loop ends doing nothing
    //        - if a new RX1 package has arrived (ISR has increment BeeIotRXFlag): process RX Queue as usual.


	if(rc == CMD_ACKRX1){	// Got ACK with request to open RX1 wait window
		// Release TX Session buffer: reset ACK & Retry flags again
		MyMsg.ack     = 0;    // probably for a RETRY session by BIoTParse()
		MyMsg.retries = 0;

		// 11. Start RX1 Window: TX session Done (ACK received):
		//  Give the GW an option for another Job
		BeeIoTStatus  = BIOT_RX;  // enter RX1 window session
		LoRa.receive(0);          // Activate: RX-Continuous in expl. Header mode

		ackloop=0;                // reset wait counter
		BHLOG(LOGLORAW) Serial.printf("\n  LoRaLog: wait for add. RX1 Pkg. (RXCont):");
		while (!BeeIotRXFlag & (ackloop < WAITRX1PKG*2)){  // wait till RX pkg arrived or max. wait time is over
			BHLOG(LOGLORAW) Serial.print("o");
			mydelay2(500,0);             // wait time (in msec.) -> frequency to check RXQueue: 0.5sec
			ackloop++;
		}
	}

	if(rc == CMD_ACK){	// Normal ACK received -> expect no RX1 window requested
		rc=0;	 		// mark: Bypass RX1 window
	}
	// 12. ACK TO condition reached
	else if(ackloop >= WAITRX1PKG*2){ // Yes, no RX1 pkg received -> we are done
        // nothing else to do: no ACK -> no RX Queue handling needed
        BHLOG(LOGLORAW) Serial.println(" None.\n");
        // RX Queue should still be empty: just check it
        if(!BeeIotRXFlag & (RXPkgSrvIdx != RXPkgIsrIdx)){ // RX Queue realy empty ?
          Serial.printf("  LoRaLog: This case should never happen: Queue-RD(%i)/WR(%i) Index different when BeeIoTRXFlag==0\n",RXPkgSrvIdx, RXPkgIsrIdx);
          RXPkgSrvIdx = RXPkgIsrIdx;  // Fix: Force empty Queue entry: RD & WR ptr. must be identical
          // ToDo: any further correction action ? by now: show must go on.
        }
        rc=0; // We do not expect a RX1 job. ACK-TO is no problem -> go on with positive return code
	}
    // 13. Either RX1 job received
    // or RETRY / REJOIN as Ack from previous TX session
	else  if(rc == CMD_REJOIN){
	  // 14. Process REJOIN Request from GW
      // For REJOIN we have to keep JOIN Request status (as set by BeeIoTParse())
      BHLOG(LOGLORAR) Serial.printf("  LoRaLog: Send Msg failed - New-Join requested, RX Queue Status: SrvIdx:%i, IsrIdx:%i, NextMsgID:%i, RXFlag:%i\n",
                         RXPkgSrvIdx, RXPkgIsrIdx, LoRaCfg.msgCount, BeeIotRXFlag);
      BeeIoTStatus = BIOT_JOIN;       // RE-JOIN requested by GW -> to be process as new JOIN command on default Join NodeID
      LoRaCfg.nodeid = NODEIDBASE;    // -> CONFIG response from GW will provide a new MsgID (roaming ?!) later
      LoRa.sleep();                   // stop modem and clear FIFO, but keep JOIN mode
      BHLOG(LOGLORAW) Serial.printf("\n  LoraLog: Enter JOIN-Request Mode\n");
	  sprintf(bhdb.dlog.comment, "ReJoin");
      return(-96);
	}
	else {  // Simply a new pkg received -> process it
      BHLOG(LOGLORAW) Serial.printf("  LoRaLog: add. Pkg received: %s\n", beeiot_ActString[MyRXData[RXPkgSrvIdx].hd.cmd]);
      BHLOG(LOGLORAR) Serial.printf("  LoRaLog: RX Queue Status: SrvIdx:%i, IsrIdx:%i, new PkgID:%i, RXFlag:%i\n",
                        RXPkgSrvIdx, RXPkgIsrIdx, MyRXData[RXPkgSrvIdx].hd.pkgid, BeeIotRXFlag);

      // 14. Parse RX1 pkg: ISR has checked Header and copied pkg into MyRXData[RXPkgSrvIdx]
      rc = BeeIoTParse(&MyRXData[RXPkgSrvIdx]); // parse and initiate action if possible
      if(rc < 0){
        BHLOG(LOGLORAW) Serial.printf("  LoRaLog: parsing of Msg[%i]failed rc=%i\n", MyRXData[RXPkgSrvIdx].hd.pkgid, rc);
        // Detailed error analysis/message done by BeeIoTParse() already
      }
      // if rc == CMD_RETRY -> Last TX pkg payload was corrupt: BeeIoTParse has resend it again already
      // -> loop for ACK-wait again

      // Adjust RX Queue RD ptr (Idx): as ring buffer (if end reached, start at the beginning)
      if(++RXPkgSrvIdx >= MAXRXPKG){  // RX Queue buffer end reached ?
        BHLOG(LOGLORAR) Serial.printf("  LoRaLog: RX Buffer end reached, switch back to buffer[0]\n");
        RXPkgSrvIdx=0;                // wrap around (but stay behind Write Index from ISR !)
      }

      BeeIotRXFlag--;                 // and we can release one more buffer in the RX Queue
      if(!BeeIotRXFlag & (RXPkgSrvIdx != RXPkgIsrIdx)){ // RX Queue validation check: realy empty ?
        Serial.printf("  LoRaLog: This case should never happen 2: Queue-RD(%i)/WR(%i) Index different when BeeIoTRXFlag==0\n",
              RXPkgSrvIdx, RXPkgIsrIdx);
        RXPkgSrvIdx = RXPkgIsrIdx;    // Fix: Force empty Queue entry: RD & WR ptr. must be identical
        // ToDo: any further correction action ? by now: show must go on.
      }
    } // New Pkg processed

  } while(rc == CMD_RETRY);  // end of (RX1-)ACK wait loop: continue if CMD_RETRY was processed

   // clean up TX Msg buffer (ISR checks for MyMsg.idx == RXData.index only)
  MyMsg.ack     = 0;
  MyMsg.idx     = 0;
  MyMsg.retries = 0;
  MyMsg.pkg     = (beeiotpkg_t *) NULL;  // TX done -> release LoRa package buffer
  LoRaCfg.msgCount++;           // increment global sequ. TX package/message ID

  BeeIoTSleep();  // we are Done with current session! LoRa-Sleep till next command

  BHLOG(LOGLORAR) Serial.printf("  LoRaLog: Msg sent done, RX Queue Status: SrvIdx:%i, IsrIdx:%i, NextMsgID:%i, RXFlag:%i\n",
        RXPkgSrvIdx, RXPkgIsrIdx, LoRaCfg.msgCount, BeeIotRXFlag);
  return(rc);
}


//******************************************************************************
// WaitForAck()
// Wait for LoRa pck: ACK on last send Lora Msg
// Input:
//	BIOTStatus	Mode of Lora Transfer state machine
//	Msg			Protocol status of last sent LoRa message
// 	Modem		Detected RFM95 LoRa device
// Return:
//	= CMD_ACK	Ack received + RX1 requested by GW
//  = CMD_ACKNORX1 ACK received + no RX1 window requested by GW
//  = -99		Max # of retries reached -> No Ack received
//				=> Modem entered JOIN mode again
// Global:
//	RXPkgSrvIdx, RXPkgIsrIdx, BeeIotRXFlag	-> LoraPakg Queue
//              received LoRa package can be found in: MyRXData[RXPkgSrvIdx]
//	bhdb		BIoT status DB
//******************************************************************************
int WaitForAck(byte *BIOTStatus, beeiotmsg_t *Msg, beeiotpkg_t * Pkg, LoRaClass *Modem){
	int ackloop;  // ACK wait loop counter

    // Activate flow control: RX Continuous in expl. Header mode
    *BIOTStatus = BIOT_RX;           // start RX session
    Modem->receive(0);				 // prepare RFM95 chip for receiving a LoRa IRQ in RXCont Mode

    BHLOG(LOGLORAW) Serial.printf("  LoRaLog: wait for incoming ACK in RXCont mode (Retry: #%i)", Msg->retries);
    mydelay2(MSGRESWAIT,0);          // min. wait time for ACK to arrive -> polling rate

    ackloop=0;                       // preset RX-ACK wait loop counter
    while(Msg->ack == 0){            // wait till TX got committed by ACK-info => set by LoRa-ISR routine

      // 8. No ACK received -> Check for ACK Wait Timeout
      if(ackloop++ > MAXRXACKWAIT){   // max # of wait loops reached ? -> yes, ACK RX timed out
        BHLOG(LOGLORAW) Serial.printf("timed out\n");

        if(Msg->retries < MSGMAXRETRY){      // policy for one more Retry ?
          // 9. Initiate a retry loop & and start RXCont for ACK pkg again
          // ToDo: Set up Timer watchdog for TXDone wait and act accordingly
          Msg->retries++;                    // remember # of retries
          *BIOTStatus = BIOT_TX;             // start next TX session
          while(!sendMessage(Pkg, 0));       // Send same Pkg /w same msgid (!) in sync mode again

          // Wait for ACK again
          *BIOTStatus = BIOT_RX;             // Mark: start RX session
          Modem->receive(0);                 // Activate: RX-Continuous in expl. Header mode for next expected ACK
          BHLOG(LOGLORAW) Serial.printf("  LoRaLog: waiting for ACK in RXCont mode (Retry: #%i)", Msg->retries);
          ackloop=0;                         // reset ACK wait loop counter

        }else{
          // 10. Max. # of Retries reached
          // -> give up: goto sleep mode and clear MyMsg TX buffer
          // => Set REJOIN Status: at next TX session we first ask for a GW to avoid further ACK loops
          BHLOG(LOGLORAW) Serial.printf("  LoRaLog: Msg(#%i) Send-TimedOut; RX Queue Status: SrvIdx:%i, IsrIdx:%i, Length:%i\n",
                  Msg->idx, RXPkgSrvIdx, RXPkgIsrIdx, BeeIotRXFlag);

          // clean up TX Msg buffer (ISR checks for MyMsg.idx == RXData.index only)
          Msg->ack     = 0;
          Msg->idx     = 0;
          Msg->retries = 0;
          Msg->pkg     = (beeiotpkg_t *)NULL; // TX skipped -> release LoRa package buffer

          // Set REJOIN: keep msgCount but receive new ChannelCfg data from GW
          *BIOTStatus = BIOT_REJOIN;         // for next TX job: first ask for a GW in range
          LoRaCfg.msgCount++;                // increment global sequ. TX package/message ID for next TX pkg
          BHLOG(LOGLORAR) Serial.printf("\n  LoraLog: Sleep Mode\n");
          Modem->sleep();                    // Sleep Mode: Stop modem, clear FIFO -> save power
		  sprintf(bhdb.dlog.comment, "AckTO");

          return(-99);                       // give up and no RX Queue check needed neither -> GW dead ?
        } // if(ACK-TO & Max-Retry)

      } // if(ACK-TO)

  	  BHLOG(LOGLORAW) Serial.print(".");
      mydelay2(MSGRESWAIT,0);              // min. wait time for ACK to arrive -> polling rate

    } // while(no ACK)

	return(Msg->ack);
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
ackbcn_t * ackbcn;  // ptr. on ack data of beacon
beeiot_cmd_t * rx1pkg;
beeiot_sddir_t* sxpkg;
int rc;

// We have a new valid BeeIoT Package (header and data length was checked already):
// check the "msg->cmd" for next action from GW
	switch (msg->hd.cmd){
	case CMD_NOP:
		// intended to do nothing -> test message for communication check
		// for test purpose: dump payload in hex format
    	BHLOG(LOGLORAW) Serial.printf("  BeeIoTParse[%i]: cmd= NOP received\n", msg->hd.pkgid);
		rc= CMD_NOP;
		break;
	case CMD_JOIN:
		// intended to do nothing -> typically send from Node -> GW only.
    	BHLOG(LOGLORAW) Serial.printf("  BeeIoTParse[%i]: cmd= JOIN received from sender: 0x%02X\n", msg->hd.pkgid, msg->hd.sendID);
		rc= CMD_JOIN;
		break;

	case CMD_REJOIN:
		// GW requested a REJOIN
		BHLOG(LOGLORAW) Serial.printf("  BeeIoTParse[%i]: cmd= REJOIN received from sender: 0x%02X\n", msg->hd.pkgid, msg->hd.sendID);
		BeeIoTStatus = BIOT_REJOIN;  	// reset Node to JOIN Reqeusting Mode -> New Join with next LoRaLog request by Loop()
		report_interval = 120;			  // retry JOIN after 2 Minute; will be upd. by Config Pkg. at JOIN
		rc= CMD_REJOIN;
		break;

	case CMD_CONFIG:                           // new BeeIoT channel cfg received
		BHLOG(LOGLORAW) Serial.printf("  BeeIoTParse[%i]: cmd= CONFIG -> switch to new channel cfg.\n", msg->hd.pkgid);
		// ToDo: action: parse channel cg. data and update channel cfg. table
		rc=BeeIoTParseCfg((beeiot_cfg_t*) msg);  // lets parse received message as Switch CFG request
		break;

	case CMD_LOGSTATUS: // BeeIoT node sensor data set received
		BHLOG(LOGLORAW) Serial.printf("  BeeIoTParse[%i]: cmd= LOGSTATUS -> should be ACK => ignored\n", msg->hd.pkgid);
		// intentionally do nothing on node side
		rc=CMD_LOGSTATUS;
		break;

	case CMD_DSENSOR: // BeeIoT node sensor binary data set received
		BHLOG(LOGLORAW) Serial.printf("  BeeIoTParse[%i]: cmd= DSENSOR -> should be ACK => ignored\n", msg->hd.pkgid);
		// intentionally do nothing on node side
		rc=CMD_LOGSTATUS;
		break;

	case CMD_GETSDDIR:
	    BHLOG(LOGLORAW) Serial.printf("  BeeIoTParse[%i]: SDDIR Request command: not completed yet\n", msg->hd.pkgid);
		// for test purpose: dump payload in hex format
			// ToDo: read out SD card and send it package wise to GW
		rx1pkg = (beeiot_cmd_t *) msg;
		sxpkg = (beeiot_sddir_t*) &MyTXData;
		// cmdp1: Directory level - 1: '/' Root directory
		// cmdp2: Max. Number of requested text strings (or index == 0xFF earlier)
		// cmdp2: not used
		sxpkg->sddir_seq = 1;	// by now only 1 string with index 1
		sd_get_dir(rx1pkg->p.cmdp1, rx1pkg->p.cmdp2, rx1pkg->p.cmdp3, (char *) sxpkg->sddir);

		// Prepare new BeeIoT TX-package session
		LoRaCfg.msgCount++;            // increment global sequ. TX package/message ID for next TX pkg
		sxpkg->hd.cmd = CMD_GETSDDIR;	// Provide new Directory Text line
		sxpkg->hd.destID = LoRaCfg.gwid;     // remote target GW
		sxpkg->hd.sendID = LoRaCfg.nodeid;   // that's me
		sxpkg->hd.pkgid  = LoRaCfg.msgCount; // serial number of sent packages (sequence checked on GW side!)
		sxpkg->hd.frmlen = sizeof(uint8_t) + strlen(sxpkg->sddir)+1; // length of user payload data incl. any EOL - excl. MIC
		sxpkg->hd.res	 = 0;
		// Add MIC (4By.) to end of payload data
  		BIoT_getmic( & MyTXData, UP_LINK, (byte*) & MyTXData.data[MyTXData.hd.frmlen]); // MIC CMAC generation of (TX-hdr + TX-payload)

  		// remember new Msg (for mutual RETRIES and ACK checks)
  		// have to do it before sendmessage -> used by ISR for TX-ACK response
		MyMsg.ack     = 0;                  // in sync mode: checked by sendMessage() and set by ISR
		MyMsg.idx     = LoRaCfg.msgCount;
		MyMsg.retries = 0;                  // TX retry counter in transmission error case
		MyMsg.pkg     = (beeiotpkg_t*) & MyTXData;  // remember TX Message of session

  		// Initiate TX session
  		BeeIoTStatus = BIOT_TX;             // start TX session
  		while(!sendMessage(&MyTXData, 0));  // send it in sync mode: wait for RXDone

		rc=CMD_GETSDDIR;                        // -> No wait for ACK by upper layer required
		break;

	case CMD_GETSDLOG:
	    BHLOG(LOGLORAW) Serial.printf("  BeeIoTParse[%i]: SDLOG Data command: not completed yet\n", msg->hd.pkgid);
		// for test purpose: dump payload in hex format
		rx1pkg = (beeiot_cmd_t *) msg;		// optional: rx1pkg->p.cmdp1..3
		// ToDo: read out SD card and send it package wise to GW
		// cmdp1:  Max. Number of requested text strings (or index == 0xFF earlier)
		// cmdp2: not used
		// cmdp3: not used
		rc= CMD_GETSDLOG;                      // not supported yet
		break;

	case CMD_RESET:
		// Reset all statistic counter, clear SD and initiate JOIN for new cfg. data
		rx1pkg = (beeiot_cmd_t *) msg;		// optional: rx1pkg->p.cmdp1..3
	    BHLOG(LOGLORAW) Serial.printf("  BeeIoTParse[%i]: RESET Node: %d, %d, %d\n", msg->hd.pkgid, rx1pkg->p.cmdp1, rx1pkg->p.cmdp2, rx1pkg->p.cmdp3);

		// cmdp1: Reset Level: 1: All (Lora + SD), 2: Lora only
		// cmdp2: SDLevel 1: Reset-File, 2: Reset LogPathFile
		// cmdp3: not used
		if(rx1pkg->p.cmdp1 == 1){ // reset all areas
			ResetNode(rx1pkg->p.cmdp1, rx1pkg->p.cmdp2, rx1pkg->p.cmdp3);		// do everything on sensor node side here...
		}

		// Reset LoRa:  Reset Statistic, Setup RX Queue management, enter JOIN Mode
		BeeIotRXFlag= 0;              // reset Semaphor for received message(s) -> polled by Sendmessage()
		RXPkgSrvIdx = 0;              // preset RX Queue Read Pointer
		RXPkgIsrIdx = 0;              // preset RX Queue Write Pointer
		LoRaCfg.joinCount = 0;
		LoRaCfg.joinRetryCount = 0;
		LoRaCfg.msgCount = 0;

		BeeIoTStatus = CMD_JOIN;	  // start new GW join again.

		rc = CMD_REJOIN;
		break;

	case CMD_RETRY:
		BHLOG(LOGLORAW) Serial.printf("  BeeIoTParse[%i]: RETRY-Request: send same message again\n", msg->hd.pkgid);
		if(MyMsg.pkg->hd.pkgid == msg->hd.pkgid){    // do we talk about the same Node-TX message we are waiting for ?
			BeeIoTStatus = BIOT_TX;              // Retry Msg -> in TX mode again
			while(!sendMessage(&MyTXData, 0));   // No, send same pkg /w same msgid + MIC in sync mode again
			MyMsg.retries++;                     // remember # of retries
			rc=CMD_RETRY;                        // -> wait for ACK by upper layer required
		}else{
			BHLOG(LOGLORAW) Serial.printf("  BeeIoTParse: RETRY but new MsgID[%i] does not match to TXpkg[%i] ->ignored\n",
					msg->hd.pkgid, MyMsg.pkg->hd.pkgid);
			rc = -1;  // retry requested but not for last sent message -> obsolete request
		}
		break;

	case CMD_ACK:	// we should not reach this point -> covered by ISR
	case CMD_ACKRX1:
		BHLOG(LOGLORAW) Serial.printf("  BeeIoTParse[%i]: ACKRX1 received -> Should have been processed by ISR ?!\n", msg->hd.pkgid);
		//  No action: Already done by onReceive() implicitely
		// BHLOG(LOGLORAW) Serial.printf("  BeeIoTParse[%i]: ACKRX1 received -> Data transfer done.\n", msg->index);
		if(MyMsg.pkg->hd.pkgid == msg->hd.pkgid){    // save link to User payload data
			MyMsg.ack = CMD_ACK;                     // confirm Acknowledge flag
		}
		rc = CMD_ACK;   // Message sending got acknowledged
		break;

	case CMD_ACKBCN:   // Beacon Ack Pack received
    	// BHLOG(LOGLORAW) Serial.printf("  BeeIoTParse[%i]: ACK received -> Data transfer done.\n", msg->index);
		BHLOG(LOGLORAR) hexdump((unsigned char*) msg, BIoT_HDRLEN + msg->hd.frmlen + BIoT_MICLEN);
		if(MyMsg.pkg->hd.pkgid == msg->hd.pkgid){    // save link to User payload data
			MyMsg.ack = CMD_ACK;                     // confirm Acknowledge info
		}
		ackbcn = (ackbcn_t*) & msg->data;   // get ptr. on beaon ack data
		bhdb.rssi = ackbcn->rssi; // save RSSI for EPD
		bhdb.snr  = ackbcn->snr;  // save SNR  for EPD
		BHLOG(LOGLORAW) Serial.printf("  BeeIoTParse[%i]: ACKBCN(len:%iBy) - RSSI:%i  SNR:%i\n", msg->hd.pkgid, msg->hd.frmlen, ackbcn->rssi, ackbcn->snr);

		// ToDo: print Beacon ack data to EPD

		rc = CMD_ACK;   // Message sending got acknowledged like normal ACK
		break;

	default:
		BHLOG(LOGLORAW) Serial.printf("  BeeIoTParse[%i]: unknown CMD(%d)\n", msg->hd.pkgid, msg->hd.cmd);
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

    if(MyMsg.pkg->hd.pkgid != pcfg->hd.pkgid){    // do we talk about the same Node-TX message we are waiting for ?
      return(-1);
    }
    // parse CFG struct here and configure modem
    LoRaCfg.nodeid    = pcfg->cfg.nodeid;
    LoRaCfg.gwid      = pcfg->cfg.gwid;
    LoRaCfg.msgCount  = pcfg->cfg.nonce;

// update next Modem Config settings (gets activated at next LoRa Pckg Send via configLoraModem() call)
#ifdef BEACON
    SetChannelCfg(BEACONCHNCFG);          		// initialize LoraCfg field by new cfg
    report_interval = BEACONFRQ;                // sec. frequency of beacon reports via LoRa
#else
    SetChannelCfg(pcfg->cfg.channelidx);        // initialize LoraCfg field by new cfg
    report_interval = pcfg->cfg.freqsensor*60;  // min -> sec. frequency of sensor reports via LoRa
#endif
	bhdb.chcfgid = pcfg->cfg.channelidx;		// save channelidx for epd print
	bhdb.woffset = pcfg->cfg.woffset;			// offset for weight scale adjustement
	bhdb.hwconfig= pcfg->cfg.hwconfig;			// save HW component enable eflag field

//    lflags = (uint16_t) pcfg->cfg.verbose;	// get verbose value for BHLOG macro; needs to be 2 byte

    // yearoff => setRTCtime() expects base to 1900
    setRTCtime(pcfg->cfg.yearoff+100, pcfg->cfg.month-1, pcfg->cfg.day, pcfg->cfg.hour, pcfg->cfg.min, pcfg->cfg.sec);

    BHLOG(LOGLORAW) Serial.printf("  BeeIoTParseCfg: BIoT-Interval: %isec., Verbose:%i, ChIndex:%i, NDID:0x%02X, GwID:0x%02X, MsgCnt:%i, HWConf:0x%02X\n",
      report_interval, lflags, LoRaCfg.chcfgid, LoRaCfg.nodeid, LoRaCfg.gwid, LoRaCfg.msgCount, bhdb.hwconfig);
    BHLOG(LOGLORAW) Serial.printf("  BeeIoTParseCfg: Received GW-Time: %i-%02i-%02i %02i:%02i:%02i\n",
      2000+pcfg->cfg.yearoff, pcfg->cfg.month, pcfg->cfg.day, pcfg->cfg.hour, pcfg->cfg.min, pcfg->cfg.sec);

    rc=CMD_CONFIG;
  return(rc);
}
//******************************************************************************
// SetChannelCfg()
// initialize LoraCfg-struct by Channel cfg settings by given Channel cfg id
// Input:
//  channelidx   index on channelconfig table
// Return: rc
//	0	      Channel configuration reset to given channelidx
//
//******************************************************************************
int SetChannelCfg(byte  channelidx){
channeltable_t * pchcfg;

// update next Modem Config settings (gets activated at next configLoraModem() call)
    LoRaCfg.chcfgid = channelidx;

	pchcfg = & cfgchntab[LoRaCfg.chcfgid];
    LoRaCfg.freq = pchcfg->frq;
    LoRaCfg.bw   = pchcfg->band;
    LoRaCfg.sf   = pchcfg->sfbegin;
    LoRaCfg.cr   = pchcfg->cr;
    LoRaCfg.pw   = pchcfg->pwr;
return(0);
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
  BHLOG(LOGLORAR) Serial.printf("  LoRaSend: TXData <PkgLen= %dBy>\n", TXData->hd.frmlen+BIoT_HDRLEN+BIoT_MICLEN);

  // start package creation:
  if(!LoRa.beginPacket(0))                // reset FIFO ptr.; in explicit header Mode
    return(0);                            // still transmitting -> have to come back later


// Serial.flush(); // send all Dbg msg up to here...
  // now lets fill up FIFO:
  LoRa.write(TXData->hd.destID);          // add destination address
  LoRa.write(TXData->hd.sendID);          // add sender address
  LoRa.write(TXData->hd.pkgid);           // add message ID = package serial number
  LoRa.write(TXData->hd.cmd);             // store function/command for GW
  LoRa.write(TXData->hd.frmlen);          // user-payload = App Frame length
  LoRa.write(TXData->hd.res);             // user-payload = App Frame length

  byte len = (byte) TXData->hd.frmlen + BIoT_MICLEN;
  LoRa.write((byte*) & TXData->data, len);          // add AppFrame incl. MIC bytewise

	BHLOG(LOGLORAR) Serial.println("  sendMessage: Start TX");

  // Finish packet and send it in LoRa Mode
  // =0: sync mode: endPacket returns when TX_DONE Flag was achieved
  // =1 async: polling via LoRa.isTransmitting() needed; -> remove LoRa.idle()
  LoRa.endPacket(async);

  BHLOG(LOGLORAR) Serial.printf("  LoRaSend(0x%x>0x%x)[%i](cmd=%d) <FrmLen: %iBy>\n",
          TXData->hd.sendID, TXData->hd.destID, TXData->hd.pkgid, TXData->hd.cmd,
          TXData->hd.frmlen);
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


    BHLOG(LOGLORAW) Serial.println();
	BHLOG(LOGLORAW) Serial.print("    LoRa-IRQ PkgLen: ");
	BHLOG(LOGLORAW) Serial.println(packetSize);
	rtc_wdt_feed();		// Feed the IRQ WD Timer -> get add. time for ISR here...

    if(BeeIotRXFlag >= MAXRXPKG){ // Check RX Semaphor: RX-Queue full ?
    // same situation as: RXPkgIsrIdx+1 == RXPkgSrvIdx (but hard to check with ring buffer)
    	  BHLOG(LOGLORAR) Serial.printf("onReceive: InQueue full (%i items)-> ignore new IRQ with Len 0x%x\n", (byte) BeeIotRXFlag, (byte) packetSize);
        return;     // User service must work harder
    }

  // is it a package size conform to BeeIoT WAN definitions ?
  if (packetSize < BIoT_HDRLEN || packetSize > MAX_PAYLOAD_LENGTH) {
    BHLOG(LOGLORAR) Serial.printf("onReceive: New RXPkg exceed BeeIoT pkg size: len=0x%x\n", packetSize);
    return;          // no payload of interest for us, ignore it.
  }
 // return;

  // from here on: assumed explicite header mode: -> Size from = REG_RX_NB_BYTES Bytes
  // read expected packet user-header bytes leading BeeIoT-WAN header
  ptr = (byte*) & MyRXData[RXPkgIsrIdx];

  for(count=0; count < packetSize; count++){      // as long as FiFo Data bytes left
    ptr[count] = (byte)LoRa.read();     // read bytes one by one for REG_RX_NB_BYTES Bytes
  }
  msg = & MyRXData[RXPkgIsrIdx];  // Assume: BeeIoT-WAN Message received
//  BHLOG(LOGLORAR) hexdump((unsigned char*) msg, BIoT_HDRLEN);

// now check real data:
// start the gate keeper of valid BeeIoT Package data:  ( check for known GW or JOIN GW ID)
	if((msg->hd.sendID != LoRaCfg.gwid) && (msg->hd.sendID != GWIDx)){
		BHLOG(LOGLORAR) Serial.printf("onReceive: Unknown GWID detected (0x%02X) -> package rejected\n", (unsigned char)msg->hd.sendID);
		return;
	}
	if(msg->hd.destID != LoRaCfg.nodeid){
		BHLOG(LOGLORAR) Serial.printf("onReceive: Wrong Target NodeID detected (0x%02X) -> package rejected\n", (unsigned char)msg->hd.destID);
		return;
	}
  // Expected pkg format: Header<NODEID + GWID + index + cmd + frmlen> + AppFrame + MIC
	if(BIoT_HDRLEN+msg->hd.frmlen+BIoT_MICLEN != packetSize){
		BHLOG(LOGLORAR) Serial.printf("onReceive: Wrong payload size detected (%i) -> package rejected\n", packetSize);
		return;
	}

  // At this point we have a new header-checked BeeIoT Package
  // some debug output: assumed all is o.k.
	BHLOG(LOGLORAR) Serial.printf("onReceive: RX(0x%02X>0x%02X)", (unsigned char)msg->hd.sendID, (unsigned char)msg->hd.destID);
	BHLOG(LOGLORAR) Serial.printf("[%i]:(cmd=%i: %s) ", (unsigned char) msg->hd.pkgid, (unsigned char) msg->hd.cmd, beeiot_ActString[msg->hd.cmd]);
	BHLOG(LOGLORAR) Serial.printf("DataLen=%i Bytes\n", msg->hd.frmlen);

  // if it is CMD_ACK pkg -> lets commit to waiting TX-Msg
  if( (msg->hd.cmd == CMD_ACK) || (msg->hd.cmd == CMD_ACKRX1)){
    if(msg->hd.pkgid == MyMsg.idx){ // is last TX task waiting for this ACK ?
      MyMsg.ack = msg->hd.cmd;       	// ACK info hopefully still polled by waiting TX service
    }else{                        	// noone is waiting for this ACK -> ignore it, but leave a note
      BHLOG(LOGLORAR) Serial.printf("onReceive: ACK received -> with no matching msg.index: %i\n", msg->hd.pkgid);
    } // skip RX Queue handling for ACK pkgs.

  }else{  // for any other CMD of RX package -> lets do it on userland side
    // keep ACKBCN, RETRY or REJOIN as ACK to satisfy ACK wait loop at LoRaLog()
    if((msg->hd.cmd == CMD_RETRY) || (msg->hd.cmd == CMD_REJOIN) || (msg->hd.cmd == CMD_ACKBCN)){
      if(msg->hd.pkgid == MyMsg.idx){  // is the last TX task waiting for an ACK  but got RETRY/Rejoin now ?
        MyMsg.ack = msg->hd.cmd;       // report new cmd -> parsed by waiting TX service
      }else{                           // noone is waiting for an ACK -> but we could process it anyway
        BHLOG(LOGLORAR) Serial.printf("onReceive: RETRY/REJOIN/ACKBCN received -> with no matching msg.index: %i\n", msg->hd.pkgid);
		MyMsg.ack = 0;				   // ignore: no valid Ack command; just another command package we are not waiting for
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
	// print ISR notes in real time
  	// Serial.flush();

  return;
} // end of RX ISR process


//****************************************************************************************
// BeeIoTBeacon()
// prepare beacon package and send it
// Global: MyTXData[] casted by bcn ptr.
// INPUT
//  bcnmode  = BIOT_JOIN: Send Beacon instead of JOIN request to default JOIN GW
//           !=BIOT_JOIN: Send beacon as normal CMD pkg to joined dGW
// Return:
//   0:  beacon sent and ack received
//  -1:  beacon sent failed
//****************************************************************************************
int BeeIoTBeacon(int bcnmode){
// beeiot_cfg_t * pcfg;      // ptr on global Node settings
beeiot_beacon_t * pbcn;   // ptr. on Beacon message format field (reusing global MyTXData buffer)
beacon_t * dbcn;           // ptr on a beacon data frame
int i;
int rc;

// 1. Check for a valid LoRa Modem Send Status
  BHLOG(LOGLORAR) Serial.printf("  BIoTBCN: BeeIoTStatus = %s\n", beeiot_StatusString[BeeIoTStatus]);
  if(BeeIoTStatus == BIOT_NONE){  // Any LoRa Modem already detected/configured ?
    if(!setup_LoRa(0))
      return(-98);                // no LoRa Modem -> no TX message
  }
  if(BeeIoTStatus == BIOT_SLEEP) {// just left (Deep) Sleep mode ?
    if(!BeeIoTWakeUp())           // we have to wakeup first
      return(-97);                // problem to wake up -> try later again
  }
  // no JOIN status check here: Beacon send should work in all cases (if no JOIN -> beacon instead of JOIN request)

  pbcn = (beeiot_beacon_t *) & MyTXData;  // fetch global TX Msg buffer in Beacon format
  dbcn = (beacon_t *) & MyTXData.data;    // and corresponding beacon data frame
  dbcn->info =0;

  // 1.Prepare Pkg Header
  pbcn->hd.cmd = CMD_BEACON;       // Send a Beacon
  if(bcnmode == BIOT_JOIN){        // not Joined yet ? -> No: do some add. settings
    pbcn->hd.sendID = NODEIDBASE;  // that's me by now but finally not checked in JOIN session
    pbcn->hd.destID = GWIDx;       // We start joining always with the default BeeIoT-WAN GWID
    LoRaCfg.nodeid  = NODEIDBASE;  // store Beacon-NodeID for onreceive() check
    LoRaCfg.msgCount= 0;           // set start seríal number of Beacon packages
    BHLOG(LOGLORAW) Serial.printf("  BIoTBCN: Send Beacon /wo JOIN\n");
    // Set Beacon channel configuration
    SetChannelCfg(LoRaCfg.chcfgid);     // initialize LoraCfg field by default modem cfg for RE/JOIN
    configLoraModem(&LoRaCfg);          // set modem to default JOIN settings
  }else{ // already joined
    pbcn->hd.sendID = LoRaCfg.nodeid;   // use nodeid of last GW cfg package
    pbcn->hd.destID = LoRaCfg.gwid;     // remote target GW
    BHLOG(LOGLORAR) Serial.printf("  BIoTBCN: Send a Beacon as Node 0x%02X\n", pbcn->hd.sendID);
  }
  pbcn->hd.pkgid   = LoRaCfg.msgCount; // ser. number of Beacon package
  pbcn->hd.frmlen  = (uint8_t) sizeof(beacon_t);   // length of beacon data - excl. MIC


  // 2.setup beacon data frame
  memcpy(dbcn->devEUI, DEVEUI, LENDEVEUI);

// TimeStamp of beacon
//  DateTime now = rtc.now(); // get current local timestamp from RTC
  dbcn->hour  = 12;     //now.hour();
  dbcn->min   = 0;      //now.minute();
  dbcn->sec   = 0;      //now.second();
  dbcn->crc1  = 11;

  // GPS data: some test values by now
  dbcn->latf  = 98.0;   // GPS latitude
  dbcn->lonf  = 99.0;   // GPS longitude
  dbcn->alt   = 634;    // Altitude = 634m
  dbcn->info  = 33;     // set generic info value
  dbcn->crc2  = 22;

  // get MIC from pkg-start to pkg-end address
  BIoT_getmic( & MyTXData, UP_LINK, (byte*) & MyTXData.data[sizeof(beacon_t)]); // MIC AES generation of (TX-hdr + TX-payload)

  // 3. remember new Msg (for mutual RETRIES and ACK checks)
  // have to do it before sendmessage -> used by ISR for TX-ACK response
  MyMsg.ack     = 0;                  // in sync mode: checked by sendMessage() and set by ISR
  MyMsg.idx     = LoRaCfg.msgCount;
  MyMsg.retries = 0;                  // TX retry counter in transmission error case
  MyMsg.pkg     = (beeiotpkg_t*) & MyTXData;  // remember TX Message of session

  // reset LoRa transmission values before requesting new one
  bhdb.rssi = 0;
  bhdb.snr  = 0;

  // 4. Initiate TX session
  // ToDo: Set up Timer watchdog for TXDone wait and act accordingly
  BeeIoTStatus = BIOT_TX;             // start TX session
  while(!sendMessage(&MyTXData, 0));  // send it in sync mode: wait for RXDone

  // Lets wait for expected CONFIG Msg
  BeeIotRXFlag=0;             // spawn ISR RX flag
  // Activate RX Contiguous flow control

    MyMsg.ack=0;                // reset ACK & Retry flags
    LoRa.receive(0);            // Activate: RX-Continuous in expl. Header mode

    i=0;  // reset wait counter
    BHLOG(LOGLORAW) Serial.printf("  BIoTBCN: waiting for ACKBCN in RXCont mode:");
    while (!BeeIotRXFlag && (i<WAITRX1PKG*2)){  // till RX pkg arrived or max. wait time is over
        BHLOG(LOGLORAW) Serial.print("o");
        mydelay2(500,0);            // count wait time in msec. -> frequency to check RXQueue
        i++;
    }

    rc=0;
    if(i>=WAITRX1PKG*2){       // TO condition reached, no RX pkg received => no GW in range ?
        BHLOG(LOGLORAW) Serial.println(" None.");
        // RX Queue should still be empty: validate it:
        if(!BeeIotRXFlag & (RXPkgSrvIdx != RXPkgIsrIdx)){ // RX Queue validation check: realy empty ?
          Serial.printf("  BIotBCN: This case should never happen: Queue-RD(%i)/WR(%i) Index different when BeeIoTRXFlag==0 -> fixed\n",RXPkgSrvIdx, RXPkgIsrIdx);
          // ToDo: any correcting action ? or exit ?
          RXPkgSrvIdx = RXPkgIsrIdx;  // no Queue entry: RD & WR ptr. must be identical
        }
        rc=-2;       // No Retry needed for beacons
    }else{  // RX pkg received: should be ACKBCN
      // ISR has checked Header and copied pkg into MyRXData[RXPkgSrvIdx] already -> now parse it
      BHLOG(LOGLORAR) Serial.printf("  BIoTBCN: RX pkg received: %s\n", beeiot_ActString[MyRXData[RXPkgSrvIdx].hd.cmd]);
      BHLOG(LOGLORAR) Serial.printf("  BIoTBCN: RX Queue Status: SrvIdx:%i, IsrIdx:%i, new PkgID:%i, RXFlag:%i\n",
              RXPkgSrvIdx, RXPkgIsrIdx, MyRXData[RXPkgSrvIdx].hd.pkgid, BeeIotRXFlag);

      rc = BeeIoTParse(&MyRXData[RXPkgSrvIdx]); // parse MyRXData[RXPkgSrvIdx] for action: should be ACKBCN
      if(rc < 0){
        BHLOG(LOGLORAW) Serial.printf("  BIotBCN: parsing of Msg[%i]failed rc=%i\n", MyRXData[RXPkgSrvIdx].hd.pkgid, rc);
        // Detailed error analysis/message done by BeeIoTParse() already
      }
        // release RX Queue buffer
      if(++RXPkgSrvIdx >= MAXRXPKG){  // no next free RX buffer left in sequential queue
        BHLOG(LOGLORAR) Serial.printf("  BIoTBCN: RX Buffer end reached, switch back to buffer[0]\n");
        RXPkgSrvIdx=0;  // wrap around
      }
      BeeIotRXFlag--;   // and we can release one more RX Queue buffer
      if((!BeeIotRXFlag) & (RXPkgSrvIdx != RXPkgIsrIdx)){ // RX Queue validation check: realy empty ?
        BHLOG(LOGLORAW) Serial.printf("  BeeIotJoin: This case should never happen 2: Queue-RD(%i)/WR(%i) Index different when BeeIoTRXFlag==0 -> fixed\n",RXPkgSrvIdx, RXPkgIsrIdx);
          // ToDo: any correcting action ? or exit ?
          RXPkgSrvIdx = RXPkgIsrIdx;  // no Queue entry: RD & WR ptr. must be identical
      }
      if(rc == CMD_REJOIN){
        // still need a joined GW before sending anything (JOIN) or GW ought to get a new JOIN (REJOIN)
        if(BeeIoTJoin(BIOT_JOIN) <= 0){
          BHLOG(LOGLORAW)  Serial.printf("  BIoTBCN: BeeIoT JOIN failed -> skipped LoRa Beacon\n");
          LoRaCfg.joinRetryCount =0;	// connection is up -> reset fail counter
        }
      }
      if(rc==CMD_ACKBCN){  // Beacon has reached GW
        rc=0;
      }
    } // New Pkg parsed

    LoRaCfg.msgCount++; // define Msgcount for next pkg (based by CONFIG Data)
    LoRa.sleep();
    BeeIoTStatus = BIOT_SLEEP;		// we are connected
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




//******************************************************************************
// print hexdump of array  in the format:
// 0xaaaa:  dddd dddd dddd dddd  dddd dddd dddd dddd  <cccccccc cccccccc>
void hexdump(unsigned char * msg, int len){
	int i, y, count;
	unsigned char c;

	if(!(msg && len)){
 		Serial.printf("  Hexdump wrong input (ptr %p, len %d)\n", msg, len);
		return;
	}

  Serial.printf("MSGfield at 0x%X:\n", (unsigned int) msg);
	Serial.printf("Address:  0 1  2 3  4 5  6 7   8 9  A B  C D  E F  length= %i Byte\n", len);
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