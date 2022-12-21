//*******************************************************************
// BeeIoTWan.h
// from Project https://github.com/mchresse/BeeIoT
//
// Description:
// BeeIoT-WAN - Lora package flow definitions
//
// Created on 08. Januar 2022, 12:58
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

using namespace std;

// BIoT Version Format: maj.min	>	starting with V1.0
// - used for protocol backward compat. and pkg evaluation
#define BIoT_VMAJOR		1		// Major version
#define BIoT_VMINOR		9		// Minor
// History:
// Version Date		Comment:
// 1.0	01.01.2020	Initial setup
// 1.1	31.03.2020	+CMD: GETTime + updated devcfg_t
// 1.2	24.04.2020	LoRaSW sync word switch; txchntab[] -> cfgchntab[]
//					PkgHD: Framelen: 2 Byte -> for 16bit payload size
//					Event CMD definition
// 1.3	02.05.2020  Add Beacon CMD + Ack
// 1.4  25.11.2020	Change I2C Bus detection from Wire -> I2DDev driver lib
//					implemented new driver for DS3231, ADS1115 & MAX123x support
//					LoRa Protocol delivers MIC value -> checked on GW side
//					Add ESP32 ChipID Detection by eFuse bitmap data
//					Shorten WAITRX1PKG Window 3->1 sec.
// 1.5  28.01.2021  Add new RESET command
// 1.6  03.04.2021	Add DSENSOR Command + biot_dsensor_t -> Sensor Data in Binary format
//					Add weight-offset to devcfg_t
// 1.7  08.01.2022  add RX-cmd + SD-dir + SD-data package definition
// 1.8  13-01-2022  Node HWConfig flag field in devcfg_t structure
// 1.9  05-10-2022  Add ACKRX1 Command to request RX1 cmd window.
//
//***********************************************

//***********************************************
// BIoT Node HW Component enable flags
// used for definition of devcfg_t->hwconfig flag field
//***********************************************
#define HC_EPD		0x01	// EPD display panel
#define HC_LORA		0x02	// LORA wireless protocol
#define HC_SDCARD	0x04	// Node SDcard for LogData & Messages
#define HC_WIFI		0x08	// Node onboard WiFi hotspot
#define HC_NTP		0x10    // NTP time update by WiFi
#define HC_BEACON	0x20	// LoRa Beacon Protocol
#define HC_HWCONF40	0x40	// HC-XXX Placeholder reserved
#define HC_HWCONF80	0x80	// HC-XXX Placeholder reserved

//***********************************************
// LoRa MAC Presets
//***********************************************
#define LENJOINEUI	8
#define LENDEVEUI	8
#define LENAPPSKEY	16
#define LENNWSKEY	16

// AppServer EUIDs:
#define BIoT_EUID	0xBB, 0xEE, 0xEE, 0xBB, 0xEE, 0xEE, 0xCC, 0xEE
#define TURTLE_EUID 0xBB, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0xAA, 0xBB
#define GH_EUID		0xBB, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07

enum _cr_t { CR_4_5=0, CR_4_6, CR_4_7, CR_4_8 };
enum _sf_t { FSK=0, SF7, SF8, SF9, SF10, SF11, SF12, SFrfu };
enum _bw_t { BW125=0, BW250, BW500, BWrfu, BW10, BW15, BW20, BW31, BW41, BW62};

typedef unsigned int  devaddr_t; // Node dynamic Device address (given by GW)
typedef unsigned char cr_t;
typedef unsigned char sf_t;
typedef unsigned char bw_t;

// LoRa defined Sync Words
#define LoRaWANSW	0x34	// LoRaWAN sync word (official)
#define LoRaPUBSW	0x12	// LoRa public WAN sync word: used for BIoTWAN

// Define LoRa Runtime parameters
#define SPREADING	SF7		// Set spreading factor (1:SF7 - 6:SF12)
#define FREQ		EU868_F1 // in Mhz! (868.1)
#define TXPOWER		14		// TX PA-Power 2..16
#define SIGNALBW	BW125	// Bandwidth (0: 125kHz)
#define CODING		CR_4_5	// Coding Rate 4/5
#define LoRaSW		LoRaPUBSW	// this constant is used for final configuration
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
// but we need it common for both sides (raw payload length  incl. MIC)
#define MAX_PAYLOAD_LENGTH 0x80 // length of LoRa Pkg in IHDMODE=0: BEEIOT_HDRLEN..0xFF

// BeeIoTWan Pkg Header and Flow control definitions
#define BIoT_HDRLEN		6	// current BeeIoT-WAN Package headerLen
#define BIoT_DLEN		MAX_PAYLOAD_LENGTH-BIoT_HDRLEN
#define BIoT_MICLEN     4	// length of NsMIC field
#define BIoT_FRAMELEN	BIoT_DLEN-BIoT_MICLEN	// length of BIoT App-Frame (encoded by AppKey) - MIC
#define	MSGRESWAIT		500	// scan each 0.5 seconds the message status in a waitloop
#define MAXRXACKWAIT	10	// # of Wait loops of MSGRESWAIT
#define MSGMAXRETRY		5	// Do it max. n times again
#define RXACKGRACETIME  10  // in ms: time to wait for sending Ack after RX pkg in BeeIoTParse()
#define MSGRX1DELAY		500	// [ms] wait for start of RX1/Cfg. window
#define WAITRX1PKG		1	// [sec] till RX1 window is closed

//*******************************************************************
// BeeIoT-WAN command package types
//*******************************************************************
// CMD: Sensor Node Command/function codes for GW action
enum {
	CMD_JOIN =0,	// JOIN Request to GW (e.g. register NodeID)
	CMD_REJOIN,		// REJOIN Request to GW (Connection got lost -> restart with Default Cfg Channel)
	CMD_LOGSTATUS,	// process Sensor log data set in ASCII Format
	CMD_GETSDLOG,	// get one more line of SD card log file from node (chunk of 116 By.)
	CMD_RETRY,		// Tell target: Package corrupt, do it again
	CMD_ACK,		// Received message completed (no RX1 window)
	CMD_CONFIG,		// exchange new set of runtime configuration parameters
	CMD_EVENT,		// Node Event, exceptional
	CMD_BEACON,		// Send beacon e.g. for distance test
	CMD_ACKBCN,		// Beacon Acknowledge -> delivers RSSI & SNR
	CMD_TIME,		// Request curr. time values from partner side
	CMD_RESET,		// Reset GW->Node: reset counter, clear SD, initiate JOIN
	CMD_NOP,		// do nothing -> for xfer test purpose
	CMD_DSENSOR,	// process Sensor log data set in binary format
	CMD_GETSDDIR,	// get one more line(string) of SD directory structure (1..254)
	CMD_ACKRX1,		// Received message completed + add RX1 Phase command to be expected
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
	[CMD_GETSDLOG]	= "GETSDDATA",
	[CMD_RETRY]		= "RETRY",
	[CMD_ACK]		= "ACK",
	[CMD_CONFIG]	= "CONFIG",
	[CMD_EVENT]		= "EVENT",
	[CMD_BEACON]	= "BEACON",
	[CMD_ACKBCN]	= "ACKBEACON",
	[CMD_TIME]		= "GETTIME",
	[CMD_RESET]		= "RESET",
	[CMD_NOP]		= "NOP",
	[CMD_DSENSOR]	= "DSENSOR",
	[CMD_GETSDDIR]	= "GETSDDIR",
	[CMD_ACKRX1]	= "ACKRX1",
};
#endif

#ifndef bool
typedef bool boolean;			// C++ type: boolan
#endif
#ifndef byte
typedef uint8_t	byte;	// 8 bit Byte == unsigned char
#endif
#ifndef u2_t
typedef uint16_t  u2_t;	// a 16 bit word = 2x byte
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
	uint8_t	destID;		// ID of receiver
	uint8_t	sendID;		// ID of BeeIoT Node sending a package
	uint8_t	pkgid;		// package index (0..0xFF)
	uint8_t	cmd;		// command code from Node
	uint8_t	frmlen;		// 8bit length of following payload Frame: BIoT_FRAMELEN (excl. MIC)
	uint8_t	res;		// reserved
} beeiot_header_t;		// length define in BIoT_HDRLEN !!! changes must be made there as well

typedef struct { // generic BeeIoT Package format
	beeiot_header_t hd;	// BeeIoT common Header
	char	data[BIoT_DLEN]; // remaining payload including MIC (last 4 uint8_t)
						// uint8_t	BIoT_NsMIC[4] pkg integrity code for complete 'beeiot_join_t' (except MIC itself)
						// cmac = aes128_cmac(NwkKey, MHDR | JoinEUI | DevEUI | DevNonce)
						// MIC = cmac[0..3]
} beeiotpkg_t;

//***************************
// (RE-)JOIN-CMD Pkg:
typedef struct {
	uint8_t	devEUI[LENDEVEUI];	// could be e.g. 0xFFFE + node board ID (extended from 6 -> 8Byte)
	uint8_t	joinEUI[LENJOINEUI];	// ==AppEUI to address the right App service behind GW+NWserver
	uint8_t	frmid[2];	// Msg=Frame idx of next app session (typ.= 1); 1. join was done with frmid=0 !
	uint8_t	vmajor;		// supported version of BeeIoTWAN protocol MSB: Major
	uint8_t vminor;		// LSB: Minor
}joinpar_t;

typedef struct {
	beeiot_header_t hd;		// BeeIoT common Pkg Header
	joinpar_t		info; 	// Join request parameters
	uint8_t			mic[BIoT_MICLEN];	// pkg integrity code for complete 'beeiot_join_t' (except MIC itself)
							// cmac = aes128_cmac(NwkKey, MHDR | JoinEUI | DevEUI | DevNonce)
							// MIC = cmac[0..3]
} beeiot_join_t;

//***************************
// CONFIG-CMD Pkg:
// TX GW side: NWserver delivers new cfg. settings -> hdr.length = sizeof(devcfg_t);
// TX Node side: node requests new config settings from NWserver; -> hdr.length = 0

typedef struct {
	// device descriptor
	u2_t	verbose;		// =0..0xffff verbose flags for debugging
	uint8_t	nodeid;			// LoRa modem unique NodeID1..n used for each Pkg
	uint8_t	gwid;			// LoRa Pkg target GWID of serving gateway for Status reports
	uint8_t	vmajor;			// major version of used/comitted BeeIoTWAN protocol
	uint8_t	vminor;			// minor version of used/comitted BeeIoTWAN protocol
	uint8_t	freqsensor;		// =1..n Sensor report frequency in [minutes]
	// LoRa Modem settings
	uint8_t	channelidx;		// used channelidx of channeltable_t	txchntab[MAX_CHANNELS]
	uint8_t	nonce;			// pkg counter init value
	uint8_t	hwconfig;		// HW configuration flag field defined by HC_xxxx masks (s.a.)

	// RTC DATETIME input format: YYoffs-MM-DD-HH-MM-SS
	uint8_t	yearoff;		// offset to 2000
	uint8_t	month;			// 1-12
	uint8_t	day;			// 1-31
	uint8_t	hour;			// 0-23
	uint8_t	min; 			// 0-59
	uint8_t	sec;			// 0-59
	int16_t woffset;		// weight offset
} devcfg_t;

typedef struct {
	beeiot_header_t hd;		// BeeIoT common Pkg Header
	devcfg_t cfg;			// node config params (must be smaller than BEEIOT_DLEN!)
	uint8_t	mic[BIoT_MICLEN]; // pkg integrity code for complete 'beeiot_cfg_t' (except MIC itself)
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

// Sensor data Stream (binary format)
// | Datentype	| Range		| ASCII	| Binary (28+1+\<text> Byte)
// |------------|-----------|-------|----------------------------------------------
// |LogID		|0 - 9999	|2397	| uint16_t xxxx
// |Datum+Zeit:	|<YYYY/MM/DD 24:59:59>|2021/03/24 22:55:52|DateTime format (RTClib.h):
// |			|			|		| uint8_t yOff  < 2000 +  yOff
// |			|			|		| uint8_t mm    < Month   1-12
// |			|			|		| uint8_t dd    < Day     1-31
// |			|			|		| uint8_t hh    < Hours   0-23
// |			|			|		| uint8_t mm    < Minutes 0-59
// |			|			|		| uint8_t ss    < Seconds 0-59
// |Gewicht		|0 - 99.99	|31.86	|  int16_t wwww	< in 10 Gramm
// |TempExtern	|0 - 99.99	|2.37	|  int16_t ccdd < in Celsius+2digits
// |TempIntern	|0 - 99.99	|4.44	|  int16_t ccdd < in Celsius+2digits
// |TempBeute	|0 - 99.99	|19.94	|  int16_t ccdd < in Celsius+2digits
// |TempRTC		|0 - 99.99	|5.25	|  int16_t ccdd < in Celsius+2digits
// |BattESP3V	|0 - 9.99	|3.36	| uint16_t vvvv < in mV
// |Board5V		|0 - 9.99	|5.00	| uint16_t vvvv < in mV
// |BattCharge	|0 - 9.99	|0.00	| uint16_t vvvv < in mV
// |BattLoad	|0 - 9.99	|3.96	| uint16_t vvvv < in mV
// |BattLevel	|0 - 100	|82		| uint8_t	pp	< in %
// |CRC8		|0 - 255	| -		| uint8_t	xx	< CRC8
// |NoticeLen	|0..16		| -		| uint8_t	xx  < char counter incl EOL
// |Notice		|<asciistream +0x00>|"o.k."| <uint8_t len><asciitext> < 15 chars max.

#define BIoT_DSENSORLEN	46	// Binary stream format: 28 + 1 + <text> Byte
#define BIoT_NOTICELEN	16	// Notice text field incl. ending 0x00
typedef struct {
			uint16_t	logid;
			uint8_t		year2k;		//< Year offset from 2000
			uint8_t		month;		//< Month 1-12
			uint8_t		day;		//< Day 1-31
			uint8_t		hh;			//< Hours 0-23
			uint8_t		mm;			//< Minutes 0-59
			uint8_t		ss;			//< Seconds 0-59
			int16_t		weight;
			int16_t		text;
			int16_t		tint;
			int16_t		thive;
			int16_t		trtc;
			uint16_t	board3v;
			uint16_t	board5v;
			uint16_t	battcharge;
			uint16_t	battload;
			uint16_t	battlevel;
			uint8_t		crc8;
			uint8_t		tlen;		// notice text len 0..15
			uint8_t		notice[BIoT_NOTICELEN];
} biot_dsensor_t;

//***************************
// RX1-CMD Pkg:	(GW -> Node)
// RX1-Request cmd Byte (in Header) + max. 3 Parameters
typedef struct {
	uint8_t	cmdp1;				// Cmd package parameter 1
	uint8_t	cmdp2;				// Cmd package parameter 2
	uint8_t	cmdp3;				// Cmd package parameter 3
} cmd_param_t;

typedef struct {
	beeiot_header_t hd;			// BeeIoT common Pkg Header incl. CMd code
	cmd_param_t	p;				// optional command parameter
	uint8_t	mic[BIoT_MICLEN];	// Cmd pkg integrity code for complete 'beeiot_sd_t' (except MIC itself)
								// cmacS = aes128_cmac(SNwkSIntKey, B1 | msg)
								// MIC = cmacS[0..3]
} beeiot_cmd_t;

//***************************
// SDDIR-Data Pkg:	(sent by request on RX1 Cmd Pkg: 'GetSDDir' cmd.
// xfer of a SD Directory structure string
#define SDDIRLEN	BIoT_FRAMELEN-1
typedef struct {
	beeiot_header_t hd;			// BeeIoT common Pkg Header
	uint8_t	sddir_seq;			// Sequence of dir string (1..max. 0xFE, 0xFF = last one)
	char	sddir[SDDIRLEN];	// raw SD directory string incl. final EOL(/0) of curr. sequence
	// MIC: pkg integrity code for header + payload (except MIC itself)
								// MIC = cmacS[0..3] = aes128_cmac(SNwkSIntKey, B1 | msg)
} beeiot_sddir_t;

//***************************
// SDLOG-Data Pkg:	(sent by request on RX1 Cmd Pkg: 'GetSDLog' cmd.
// xfer of large bin files with max. raw size: 255(MSB)*255(LSB) * (BIoT_FRAMELEN-2)
// e.g. 65535 * (0x80-6-4-2) = 7.602.060 Byte = 7.423,8kB max value (PkgLen - HDR-SQCnt-MIC)
#define SDDATLEN	BIoT_FRAMELEN-2	// length of raw data
#define BEEIOT_SDNPAR		1	// one big binary data package
typedef struct {
	beeiot_header_t hd;			// BeeIoT common Pkg Header
	uint8_t	sdseq_msb;			// MSB: data package sequence number: 0-65535 * (BIoT_FRAMELEN-2)
	uint8_t	sdseq_lsb;			// LSB data pkg. sequ. number
	char	dsd[SDDATLEN];	// raw SD data per sequ. pkg - len of 2 By. sdseq counter (msb+lsb)
	// MIC: pkg integrity code for header + payload (except MIC itself)
								// MIC = cmacS[0..3] = aes128_cmac(SNwkSIntKey, B1 | msg)
} beeiot_sd_t;

//***************************
// ACK & RETRY & NOP & RESET - CMD Pkg:
// use always std. header type: "beeiot_header_t" only with length=0

//***************************
// EVENT-CMD Pkg:
// for exceptional events;
// but normally event status is derived from LogSTatus on GW/AppSrv side
/*
enum {
	EV_BATTERY = 0,
	EV_WEIGHT,
	EV_TEMP,
	EV_GPS,
} NODEEVENT;
enum {					// Thresholds to be defined per node local type
	EV_BATT_FULL = 0,	// max. load level reached
	EV_BATT_CRITICAL,	// Batt.status reached threshold level -> charge action needed soon
	EV_BATT_EMPTY,		// Battery clos to damage -> shutdown intiated in x loops
} 	EVBATT;
enum {					// Thresholds to be defined per node local type
	EV_WEIGHT_MAX = 0,	// max. load level reached -> e.g. Honey Box full
	EV_WEIGHT_DELTA,	// Weight exceeded delta threshold -> e.g. swarm event
	EV_WEIGHT_MIN,		// Weight at min. limit -> winter food empty
} 	EVWEIGHT;
enum {
	EV_TEMP_MAX = 0,
	EV_TEMP_MIN,
} EVTEMP;
enum {
	EV_GPS_MOVE = 0,	// GPS location is changing ... node on the move -> thieved ???
} EVGPS;

#define EV_STATUSCNT 3	// number of status parameters to be parsed from user payload

typedef struct {
	beeiot_header_t hd;	// BeeIoT common Header
	char	evmaj; 		// Major Event number from NODEEVENT
	char	evmin;		// Minor event number from EVxxxx class
	char	value[4];	// Event value field: for cast to  char, word, int
	char	text[16];	// free event comment text field
} beeiot_event_t;
*/

//***************************
// beacon frame format: not used yet: t.b.d.
// BEACON-CMD Pkg:
// xfer of location Info only
// useful for distance and transmission quality tests
typedef struct {
	uint8_t	info;
	uint8_t	crc1;
	uint8_t	devEUI[LENDEVEUI];	// could be e.g. 0xFFFE + node board ID (extended from 6 -> 8Byte)
	// RTC TIME input format: HH-MM-SS
	uint8_t	hour;			// 0-23
	uint8_t	min; 			// 0-59
	uint8_t	sec;			// 0-59
	uint8_t	crc2;
	// GPS location
	union{ // latitude
		uint8_t	lat[sizeof(float)];
		float	latf;
	};
	union{ // longitude
		uint8_t lon[sizeof(float)];
		float	lonf;
	};
	uint32_t	alt;	// altitude
} beacon_t;

typedef struct {
	beeiot_header_t hd;		// BeeIoT common Pkg Header
	beacon_t bcn;			// node config params (must be smaller than BEEIOT_DLEN!)
	uint8_t	mic[BIoT_MICLEN]; // pkg integrity code for complete 'beeiot_beacon_t' (except MIC itself)
							// cmacS = aes128_cmac(SNwkSIntKey, B1 | msg)
							// MIC = cmacS[0..3]
} beeiot_beacon_t;

enum {    // Beacon frame format EU SF9
    OFF_BCN_NETID    = 0,	// 4
    OFF_BCN_TIME     = 3,	// 4
    OFF_BCN_CRC1     = 7,	// 1
    OFF_BCN_INFO     = 8,	// 1
    OFF_BCN_LAT      = 9,	// 4
    OFF_BCN_LON      = 12,	// 4
    OFF_BCN_CRC2     = 15,	// 2
    LEN_BCN          = 17	// 1
};

// CMD ACKBCN:
typedef struct {
	int		rssi;		// remaining status array
	int		snr;
} ackbcn_t;
typedef struct {
	beeiot_header_t hd;		// BeeIoT common Pkg Header
	ackbcn_t bcn;			// node config params (must be smaller than BEEIOT_DLEN!)
	uint8_t	mic[BIoT_MICLEN]; // pkg integrity code for complete 'beeiot_beacon_t' (except MIC itself)
							// cmacS = aes128_cmac(SNwkSIntKey, B1 | msg)
							// MIC = cmacS[0..3]
} beeiot_ackbcn_t;

//*****************************************************************************
// ChannelTable[]:
enum { MAX_CHANNELS = 16 };      //!< Max supported channels
enum { MAX_BANDS    =  3 };
enum { JOINCFGIDX   =  0 };		// default Channel configuration for joining: Idx  = 0
								// redefined at runtime by config.ini user value
typedef struct {
unsigned long frq;	// Frequency EU86x
bw_t	band;		// Bandwidth BWxxx
sf_t	sfbegin;	// Spreading Start SFx
sf_t	sfend;		// Spreading End   SFy
cr_t 	cr;			// Coding Rate CR_4_x
uint8_t	pwr;		// TX Power level: typ. 14
uint8_t	dc;			// duty cycle quote: 1:0,1%, 10:1%, 100:10%
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

#ifndef CHANNELTAB_EXPORT		// define this switch where txchntab should be instanciated
extern channeltable_t	cfgchntab[];	// ChannelIdx:
#else
channeltable_t	cfgchntab[MAX_CHANNELS] ={		// ChannelIdx:
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