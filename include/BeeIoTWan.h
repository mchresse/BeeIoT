//*******************************************************************
// BeeIoTWan.h  
// from Project https://github.com/mchresse/BeeIoT
//
// Description:
// BeeIoT-WAN - Lora package flow definitions
//
// Created on 17. Januar 2020, 16:13
//----------------------------------------------------------
// Copyright (c) 2019-present, Randolph Esser
// All rights reserved.
// This file is distributed under the BSD-3-Clause License
// The complete license agreement can be obtained at: 
//     https://github.com/mchresse/BeeIoT/license
// For used 3rd party open source see also Readme_OpenSource.txt
//*******************************************************************

#ifndef BEEIOTWAN_H
#define BEEIOTWAN_H

// Format: v.maj.min	>	starting with V1.0
#define BIoT_VMAJOR		1		// Major version
#define BIoT_VMINOR		0		// Minor version

//***********************************************
// LoRa MAC Presets
//***********************************************
// AppServer EUIDs:
#define BIoT_EUID	0xBB, 0xEE, 0xEE, 0xBB, 0xEE, 0xEE, 0xCC, 0xEE
#define TURTLE_EUID 0xBB, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0xAA, 0xBB
#define GH_EUID		0xBB, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07

enum _cr_t { CR_4_5=0, CR_4_6, CR_4_7, CR_4_8 };
enum _sf_t { FSK=0, SF7, SF8, SF9, SF10, SF11, SF12, SFrfu };
enum _bw_t { BW125=0, BW250, BW500, BWrfu, BW10, BW15, BW20, BW31, BW41, BW62};

typedef unsigned int  devaddr_t;		// Node dynamic Device address (given by GW)
typedef unsigned char cr_t;
typedef unsigned char sf_t;
typedef unsigned char bw_t;


// Define LoRa Runtime parameters
#define SPREADING	SF7		// Set spreading factor (1:SF7 - 6:SF12)
#define FREQ		EU868_F1 // in Mhz! (868.1)
#define TXPOWER		14		// TX PA-Power 2..16
#define SIGNALBW	BW125	// Bandwidth (0: 125kHz)
#define CODING		CR_4_5	// Coding Rate 4/5
#define LoRaWANSW	0x12	// public sync word: Other WAN:0x12, LoRa-WAN:0x34
#define LPREAMBLE   12		// 6-65535 Byte (def:12)
#define IHDMODE     0		// =1 Implcite header Mode, =0 Explicite Header Mode
#define IHEADERLEN	0		// if IH-MODE=1 => Lenght of header in Bytes 
#define NOCRC		0		// =1 NOCRC; =0 CRC check
#define RX_TOUT_LSB	0x05	// RX_TOut_LSB = 5 x Tsymbol
#define NORXIQINV	1		// =1 No Invert IQ at RX/TX

// Gateway identifier (for destID/sendID)
#define GWIDx		0x99	// Transfer ID of this gateway (default for joining)
// Node identifier (for destID/sendID)
#define NODEIDBASE	0x80
// other identifiers are defined in beelora.h => not public to all members on node side

// Basically expected to be defined in lora radio layer header file,
// but we need it common for both sides
#define MAX_PAYLOAD_LENGTH 0x80 // length of LoRa Pkg in IHDMODE=0: BEEIOT_HDRLEN..0xFF 

// BeeIoTWan Pkg Header and Flow control definitions
#define BIoT_HDRLEN	5	// current BeeIoT-WAN Package headerLen
#define BIoT_DLEN		MAX_PAYLOAD_LENGTH-BIoT_HDRLEN
#define BIoT_MICLEN     4	// length of NsMIC field
#define BIoT_FRAMELEN	BIoT_DLEN-BIoT_MICLEN	// length of BIoT App-Frame (encoded by AppKey) - MIC
#define	MSGRESWAIT		500	// scan each 0.5 seconds the message status in a waitloop
#define MAXRXACKWAIT	10	// # of Wait loops of MSGRESWAIT
#define MSGMAXRETRY		5	// Do it max. n times again
#define RXACKGRACETIME  100 // in ms: time to wait for sending Ack after RX pkg in BeeIoTParse()
#define MSGRX1DELAY		500	// [ms] wait for start of RX1 window
#define WAITRX1PKG		3	// [sec] till RX1 window is closed

//*******************************************************************
// BeeIoT-WAN command package types
//*******************************************************************
// CMD: Sensor Node Command/function codes for GW action
enum {
	CMD_JOIN =0,	// JOIN Request to GW (e.g. register NodeID)
	CMD_REJOIN,		// REJOIN Request to GW (Connection got lost -> restart with Default Cfg Channel)
	CMD_LOGSTATUS,	// process Sensor log data set
	CMD_GETSDLOG,	// one more line of SD card log file
	CMD_RETRY,		// Tell target: Package corrupt, do it again
	CMD_ACK,		// Received message complete
	CMD_CONFIG,		// exchange new set of runtime configuration parameters
	CMD_RES6,		// reserved 
	CMD_RES7,		// reserved
	CMD_RES8,		// reserved
	CMD_TIME,		// Request curr. time values from partner side 
	CMD_NOP			// do nothing -> for xfer test purpose
};
#ifndef BEEIOT_ACTSTRINGS
extern const char * beeiot_ActString[];
#else
// define BeeIoT Protocol Status/Action strings
// for enum def. -> see beelora.h
const char * beeiot_ActString[] = {
	[CMD_JOIN]		= "JOIN",
	[CMD_REJOIN]	= "REJOIN",
	[CMD_LOGSTATUS]	= "LOGSTATUS",
	[CMD_GETSDLOG]	= "GETSDLOG",
	[CMD_RETRY]		= "RETRY",
	[CMD_ACK]		= "ACK",
	[CMD_CONFIG]	= "CONFIG",
	[CMD_RES6]		= "NOP6", 
	[CMD_RES7]		= "NOP7",
	[CMD_RES8]		= "NOP8", 
	[CMD_TIME]		= "GETTIME",
	[CMD_NOP]		= "NOP"
};
#endif

#ifndef bool
typedef bool boolean;			// C++ type: boolan
#endif
#ifndef byte
typedef unsigned char	byte;	// 8 bit Byte == unsigned char
#endif
#ifndef u2_t
typedef unsigned short  u2_t;	// a 16 bit word = 2x byte
#endif

//***************************************************************
// BeeIoT CMD Packet & Frame Declarations
//***************************************************************
// BeeIoT-WAN generic packet format
// (e.g. used for casting on received LoRa MAC payloads by ISR for first header checks)
// BeeIoT LoRa Packet  		  == beeiot_header_t + data[BIoT_DLEN]
// BeeIoT LoRa Package transfer length => BIoT_HDRLEN + BIoT_FRAMELEN + BIoT_MICLEN 
// BIoT_MICLEN assures integrity of the packet range: 0..(BIoT_HDRLEN + BIoT_FRAMELEN)
typedef struct {
	byte	destID;		// ID of receiver
	byte	sendID;		// ID of BeeIoT Node sending a package
	byte	index;		// package index (0..0xFF)
	byte	cmd;		// command code from Node
	byte	frmlen;		// length of following Frame Payload: BIoT_FRAMELEN (excl. MIC)
} beeiot_header_t;

typedef struct { // generic BeeIoT Package format
	beeiot_header_t hd;	// BeeIoT common Header
	char	data[BIoT_DLEN]; // remaining payload including MIC (last 4 byte)
						// byte	BIoT_NsMIC[4] pkg integrity code for complete 'beeiot_join_t' (except MIC itself)
						// cmac = aes128_cmac(NwkKey, MHDR | JoinEUI | DevEUI | DevNonce)
						// MIC = cmac[0..3]
} beeiotpkg_t;

//***************************
// (RE-)JOIN-CMD Pkg:
typedef struct {
	byte	devEUI[8];	// could be e.g. 0xFFFE + node board ID (extended from 6 -> 8Byte)
	byte	joinEUI[8];	// ==AppEUI to address the right App service behind GW+NWserver
	byte	frmid[2];	// Frame idx of next app session (typ.= 1); 1. join was done with frmid=0 !
	byte	vmajor;		// supported version of BeeIoTWAN protocol MSB: Major
	byte 	vminor;		// LSB: Minor
}joinpar_t;

typedef struct {
	beeiot_header_t hd;		// BeeIoT common Pkg Header
	joinpar_t		info; 	// Join request parameters
	byte			mic[BIoT_MICLEN];	// pkg integrity code for complete 'beeiot_join_t' (except MIC itself)
							// cmac = aes128_cmac(NwkKey, MHDR | JoinEUI | DevEUI | DevNonce)
							// MIC = cmac[0..3]
} beeiot_join_t;

//***************************
// CONFIG-CMD Pkg:
// TX GW side: NWserver delivers new cfg. settings -> hdr.length = sizeof(devcfg_t);
// TX Node side: node requests new config settings from NWserver; -> hdr.length = 0

typedef struct {
	// device descriptor
	byte	nodeid;			// LoRa modem unique NodeID1..n used for each Pkg
	byte	gwid;			// LoRa Pkg target GWID of serving gateway for Status reports
	byte	vmajor;			// major version of used/comitted BeeIoTWAN protocol
	byte	vminor;			// minor version of used/comitted BeeIoTWAN protocol
	byte	freqsensor;		// =1..n Sensor report frequency in [minutes]
	byte	verbose;		// =0..0xff verbose flags for debugging
	// LoRa Modem settings
	byte	channelidx;		// channelidx of channeltable_t	txchntab[MAX_CHANNELS]
	byte	nonce;			// pkg counter init value
} devcfg_t;

typedef struct {
	beeiot_header_t hd;		// BeeIoT common Pkg Header
	devcfg_t cfg;			// node config params (must be smaller than BEEIOT_DLEN!)
	byte	mic[BIoT_MICLEN]; // pkg integrity code for complete 'beeiot_cfg_t' (except MIC itself)
							// cmacS = aes128_cmac(SNwkSIntKey, B1 | msg)
							// MIC = cmacS[0..3]
} beeiot_cfg_t;


//***************************
// STATUS-CMD Pkg:
#define BEEIOT_STATUSCNT 14		// number of status parameters to be parsed from user payload
typedef struct {
	beeiot_header_t hd;			// BeeIoT common Header
	char	dstat[BIoT_FRAMELEN]; 	// remaining status array
} beeiot_status_t;

//***************************
// SDLOG-CMD Pkg:
// xfer of large bin files with max. raw size: 255 * (BEEIOT_DLEN-2)
// e.g. 65535 * (0x80-5-2) = 7.929.735 Byte = 7743kB max value
typedef struct {
	beeiot_header_t hd;			// BeeIoT common Pkg Header
	byte	sdseq_msb;			// MSB: data package sequence number: 0-65535 * (BIoT_FRAMELEN-2)
	byte	sdseq_lsb;			// LSB data pkg. sequ. number
	char	dsd[BIoT_FRAMELEN-2];	// raw SD data per sequ. pkg - len of 2 By. sdseq counter
	byte	mic[BIoT_MICLEN];	// pkg integrity code for complete 'beeiot_status_t' (except MIC itself)
								// cmacS = aes128_cmac(SNwkSIntKey, B1 | msg)
								// MIC = cmacS[0..3]
} beeiot_sd_t;
#define BEEIOT_SDNPAR		1	// one big binary data package
#define BEEIOT_MAXDSD		BEEIOT_DLEN-1	// length of raw data 

//***************************
// ACK & RETRY & NOP - CMD Pkg:
// use always std. header type: "beeiot_header_t" only with length=0

enum {
    // Beacon frame format EU SF9
    OFF_BCN_NETID    = 0,         
    OFF_BCN_TIME     = 3,
    OFF_BCN_CRC1     = 7,
    OFF_BCN_INFO     = 8,
    OFF_BCN_LAT      = 9,
    OFF_BCN_LON      = 12,
    OFF_BCN_CRC2     = 15,
    LEN_BCN          = 17
};


//*****************************************************************************
// ChannelTable[]:
enum { MAX_CHANNELS = 16 };      //!< Max supported channels
enum { MAX_BANDS    =  3 };
enum { JOINCFGIDX   =  0 };		// default Channel configuration for joining: Idx  = 0

typedef struct {
unsigned long frq;	// Frequence EU86x
bw_t	band;		// Bandwidth BWxxx
sf_t	sfbegin;	// Spreading Start SFx
sf_t	sfend;		// Spreading End   SFy
cr_t 	cr;			// Coding Rate CR_4_x
byte	pwr;		// TX Power level: typ. 14
byte	dc;			// duty cycle quote: 1:0,1%, 10:1%, 100:10%
}channeltable_t;

// Default frequency plan for EU 868MHz ISM band, based on the Semtech global_conf.json defaults,
// used in The Things Network and other gateways:
//                frequence        band   datarates
enum { EU868_F1 = 868100000,      // g1   SF7-12           used during join
       EU868_F2 = 868300000,      // g1   SF7-12 SF7/250   ditto
       EU868_F3 = 868500000,      // g1   SF7-12           ditto
       EU868_F4 = 867100000,      // g2   SF7-12
       EU868_F5 = 867300000,      // g2   SF7-12
       EU868_F6 = 868800000,      // g2   FSK
       EU868_F7 = 867500000,      // g2   SF7-12  
       EU868_F8 = 867700000,      // g2   SF7-12   
       EU868_F9 = 867900000,      // g2   SF7-12   
       EU868_F10= 867900000,      // g2   SF7-12   
       EU868_DN = 869525000,      // g3   Downlink
};
enum { EU868_FREQ_MIN = 863000000, EU868_FREQ_MAX = 870000000 };
// Bands:	g1 :   1%  14dBm  
//			g2 : 0.1%  14dBm  
//			g3 :  10%  27dBm  
// Derived from Semtech techn.Note TN1300.01 "How to qualify a LoRaWAN Device in Europe":
// Initialization of BIoT Channelconfig table:

#ifndef CHANNELTAB_EXPORT		// define this switch where txchntab should be instaciated
extern channeltable_t	txchntab[];	// ChannelIdx:
#else
channeltable_t	txchntab[MAX_CHANNELS] ={		// ChannelIdx:
//Frequ.	BW	 SF-Range	CR	   TxPwr  DC
EU868_F1, BW125, SF7, SF12, CR_4_5, 14,  10,	// 0 EC-Band 48: Mandatory JOIN channel, SF7 ... SF12
EU868_F2, BW125, SF7, SF12, CR_4_5, 14,  10,	// 1 EC-Band 48: Mandatory JOIN channel, SF7 ... SF12
EU868_F2, BW250, SF7, SF7,  CR_4_5, 14,  10,	// 2 EC-Band 48: LoRa MAC Std. Channel
EU868_F3, BW125, SF7, SF12, CR_4_5, 14,  10,	// 3 EC-Band 48: Mandatory JOIN channel, SF7 ... SF12
EU868_F4, BW125, SF7, SF12, CR_4_5, 14,   1,	// 4 EC-Band 47: Lower duty cycle
EU868_F5, BW125, SF7, SF12, CR_4_5, 14,   1,	// 5 EC-Band 47:	dito
EU868_F7, BW125, SF7, SF12, CR_4_5, 14,   1,	// 6 EC-Band 47:	dito
EU868_F8, BW125, SF7, SF12, CR_4_5, 14,   1,	// 7 EC-Band 47:	dito
EU868_F9, BW125, SF7, SF12, CR_4_5, 14,   1,	// 8 EC-Band 47:	dito
0,0,0,0,0,0,0,									// 9  reserved
0,0,0,0,0,0,0,									// 10 reserved
0,0,0,0,0,0,0,									// 11 reserved
0,0,0,0,0,0,0,									// 12 reserved
0,0,0,0,0,0,0,									// 13 reserved
0,0,0,0,0,0,0,									// 14 reserved
EU868_DN, BW125, SF9, SF9,  CR_4_5, 27, 100,	// 15 EC-Band 54: GW Downlink at RX1 & RX2 -> BIoT: RX1 only
}; 			// FSK Mode is not used by BIoT
#endif


#endif /* BEEIOTWAN_H */