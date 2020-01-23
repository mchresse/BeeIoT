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

#define BEEIOTWAN_VERSION	10		// starting with V1.0 

//***********************************************
// LoRa MAC Presets
//***********************************************
// Default frequency plan for EU 868MHz ISM band, based on the Semtech global_conf.json defaults,
// used in The Things Network on Kerlink and other gateways.
//
enum { EU868_FREQ_MIN = 863000000, EU868_FREQ_MAX = 870000000 };
// Bands:	g1 :   1%  14dBm  
//			g2 : 0.1%  14dBm  
//			g3 :  10%  27dBm  
//                 freq             band     datarates
enum { EU868_F1 = 868100000,      // g1   SF7-12           used during join
       EU868_F2 = 868300000,      // g1   SF7-12 SF7/250   ditto
       EU868_F3 = 868500000,      // g1   SF7-12           ditto
       EU868_F4 = 867100000,      // g2   SF7-12
       EU868_F5 = 867300000,      // g2   SF7-12
       EU868_F6 = 868800000,      // g2   FSK
       EU868_F7 = 867500000,      // g2   SF7-12  
       EU868_F8 = 867700000,      // g2   SF7-12   
       EU868_F9 = 867900000,      // g2   SF7-12   
       EU868_DN = 869525000,      // g3   Downlink
};
enum _cr_t { CR_4_5=0, CR_4_6, CR_4_7, CR_4_8 };
enum _sf_t { FSK=0, SF7, SF8, SF9, SF10, SF11, SF12, SFrfu };
enum _bw_t { BW125=0, BW250, BW500, BWrfu, BW10, BW15, BW20, BW31, BW41, BW62};
typedef unsigned char cr_t;
typedef unsigned char sf_t;
typedef unsigned char bw_t;
typedef unsigned char dr_t;

// Define LoRa Runtime parameters
#define SPREADING	SF7		// Set spreading factor (1:SF7 - 6:SF12)
#define FREQ		EU868_F1 // in Mhz! (868.1)
#define TXPOWER		14		// TX PA-Power 2..16
#define SIGNALBW	BW125	// Bandwidth (0: 125kHz)
#define CODING		CR_4_5	// Coding Rate 4/5
#define LoRaWANSW	0x12	// public sync word: Other WAN:0x12, LoRa-WAN:0x34
#define LPREAMBLE   8		// 8 Bytes
#define IHDMODE     0		// =1 Implcite header Mode, =0 Explicite Header Mode
#define IHEADERLEN	0		// if IH-MODE=1 => Lenght of header in Bytes 
#define NOCRC		0		// =1 NOCRC; =0 CRC check
#define RX_TOUT_LSB	0x05	// RX_TOut_LSB = 5 x Tsymbol
#define NORXIQINV	1		// =1 No Invert IQ at RX/TX

// Basically expected to be defined in lora radio layer header file,
// but we need it common for both sides
#define MAX_PAYLOAD_LENGTH 0x80 // length of LoRa Pkg in IHDMODE=0: BEEIOT_HDRLEN..0xFF 

// Gateway identifier (for destID/sendID)
#define GWID1		0x99	// Transfer ID of this gateway (main sessions & join)
#define GWID2		0x98	// Transfer ID of this gateway (backup srv., SDLog, FW upd.)

// Node identifier (for destID/sendID)
#define NODEID1		0x81	// Transfer ID of LoRa Client 1
#define NODEID2		0x82	// Transfer ID of LoRa Client 2
#define NODEID3		0x83	// Transfer ID of LoRa Client 3
#define NODEID4		0x84	// Transfer ID of LoRa Client 4
#define NODEID5		0x85	// Transfer ID of LoRa Client 5

// BeeIoTWan Pkg Header and Flow control definitions
#define BEEIOT_HDRLEN	5	// current BeeIoT-WAN Package headerLen
#define BEEIOT_DLEN		MAX_PAYLOAD_LENGTH-BEEIOT_HDRLEN
#define	MSGBURSTWAIT	500	// repeat each 0.5 seconds the message 
#define MAXRXACKWAIT	10	// # of ACK Wait loops x MSGBURSTWAIT
#define MSGMAXRETRY		3	// Do it max. n times again
#define RXACKGRACETIME  1000 // in ms: time to wait for sending Ack after RX pkg in BeeIoTParse()
#define WAITRXPKG		5	// # of sec. to wait for add. RX pkg after last ACK

//*******************************************************************
// BeeIoT-WAN command package types
//*******************************************************************
// CMD: Sensor Node Command/function codes for GW action
enum {
	CMD_JOIN =0,	// JOIN Request to GW (e.g. register NodeID)
	CMD_LOGSTATUS,	// process Sensor log data set
	CMD_GETSDLOG,	// one more line of SD card log file
	CMD_RETRY,		// Tell target: Package corrupt, do it again
	CMD_ACK,		// Received message complete
	CMD_CONFIG,		// exchange new set of runtime configuration parameters
	CMD_RES6,		// reserved 
	CMD_RES7,		// reserved
	CMD_RES8,		// reserved
	CMD_RES9,		// reserved 
	CMD_NOP			// do nothing -> for xfer test purpose
};
// define BeeIoT Protocol Status/Action strings
// for enum def. -> see beelora.h
static const char * beeiot_ActString[] = {
	[CMD_JOIN]		= "JOIN",
	[CMD_LOGSTATUS]	= "LOGSTATUS",
	[CMD_GETSDLOG]	= "GETSDLOG",
	[CMD_RETRY]		= "RETRY",
	[CMD_ACK]		= "ACK",
	[CMD_CONFIG]	= "CONFIG",
	[CMD_RES6]		= "NOP6", 
	[CMD_RES7]		= "NOP7",
	[CMD_RES8]		= "NOP8", 
	[CMD_RES9]		= "NOP9",
	[CMD_NOP]		= "NOP"
};

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
// BeeIoT-WAN common package header format (e.g. for casting on received LoRa MAC payloads)
// BeeIoT LoRa Package == User-Payload[MAX_PAYLOAD_LENGTH] 
typedef struct {
	byte	destID;		// ID of receiver
	byte	sendID;		// ID of BeeIoT Node sending a package
	byte	index;		// package index (0..0xFF)
	byte	cmd;		// command code from Node
	byte	dlen;		// length of following user data
} beeiot_header_t;

typedef struct { // generic BeeIoT Package format
// ToDO: exchange header params by beiot_header_t (but also in the code)
//	beeiot_header_t hd;	// BeeIoT common Header
// char				data[BEEIOT_DLEN]; // remaining status array

	byte	destID;		// ID of receiver
	byte	sendID;		// ID of BeeIoT Node sending a package
	byte	index;		// package index (0..0xFF)
	byte	cmd;		// command code from Node
	byte	length;		// length of last status data string
	char	data[BEEIOT_DLEN]; // remaining status array
} beeiotpkg_t;

//***************************
// JOIN-CMD Pkg:
#define beeiot_nparam_join	1
typedef struct {
	byte	version;	// supported version of BeeIoTWAN protocol 
	byte	devuid[4];	// could be e.g. node board ID (as int)
}nodedescr_t;

typedef struct {
	beeiot_header_t hd;	// BeeIoT common Header
	union{
		char		djoin[BEEIOT_DLEN]; // remaining status array
		nodedescr_t info;
	};
} beeiot_join_t;

//***************************
// CONFIG-CMD Pkg:
#define beeiot_nparam_cfg	1
typedef struct {
	byte	gwid;			// LoRa Pkg ID of next serving gateway
	byte	nodeid;			// LoRa PKG NodeID1..n
	long	lorafreq;		// =EU868_F1..9,DN (EU868_F1: 868.1MHz)
	sf_t	lorasf;			// =0..8 Spreading factor FSK,7..12,SFrFu (1:SF7)
	bw_t	lorabw;			// =0..3 RFU Bandwidth 125-500 (0:125kHz)
	cr_t	loracr;			// =0..3 Coding mode 4/5..4/8 (0:4/5)
	byte	freqsensor;		// =1..n Sensor report frequency in [minutes]
	byte	verbose;		// =0..0xff verbose flags for debugging
	byte	version;		// version of used/comitted BeeIoTWAN protocol
} devcfg_t;

typedef struct {
	beeiot_header_t hd;		// BeeIoT common Header
	union{
		byte	 dcfg[BEEIOT_DLEN];	// declare max length of payload
		devcfg_t pcfg;		// node config params (must be smaller than BEEIOT_DLEN!)
	};
} beeiot_cfg_t;


//***************************
// STATUS-CMD Pkg:
typedef struct {
	beeiot_header_t hd;	// BeeIoT common Header
	char	dstat[BEEIOT_DLEN]; // remaining status array
} beeiot_status_t;
#define BEEIOT_STATUSCNT 14	// number of status parameters to be parsed from user payload

//***************************
// SDLOG-CMD Pkg:
// xfer of large bin files with max. raw size: 255 * (BEEIOT_DLEN-2)
// e.g. 65535 * (0x80-5-2) = 7.929.735 Byte = 7743kB max value
typedef struct {
	beeiot_header_t hd;	// BeeIoT common Header
	byte	sdseq_msb;			// MSB: data package sequence number: 0-65535 * (BEEIOT_DLEN-1)
	byte	sdseq_lsb;			// LSB data pkg. sequ. number
	char	dsd[BEEIOT_DLEN-2];	// raw SD data per sequ. pkg
} beeiot_sd_t;
#define BEEIOT_SDNPAR	1	// one big binary data package
#define BEEIOT_MAXDSD		BEEIOT_DLEN-1	// length of raw data 
#define BEEIOT_maxsdlen		

//***************************
// ACK & RETRY & NOP - CMD Pkg:
// use always std. header type: "beeiot_header_t" only with length=0

//******************************************************************
// For GW: TTN connectivity (optional)
//******************************************************************
// TTN Connection: TTN servers
// TODO: use host names and dns
#define TTNSERVER1  "54.72.145.119"   // The Things Network: croft.thethings.girovito.nl
#define TTNSERVER1b "54.229.214.112"  // The Things Network: croft.thethings.girovito.nl
#define TTNSERVER2  "192.168.1.10"    // local
#define TTNPORT		1700              // The port on which to send data

#endif /* BEEIOTWAN_H */