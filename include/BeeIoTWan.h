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

//***********************************************
// LoRa MAC Presets
//***********************************************

// Define LoRa Runtime parameters
#define SPREADING	SF7		// Set spreading factor (SF7 - SF12)
#define FREQ		EU868_F1 // in Mhz! (868.1)
#define TXPOWER		14		// PA-TX Power 11-20
#define SIGNALBW	BW125	// 125kHz
#define CODING		CR_4_5	// CR4/5 = 5
#define LoRaWANSW	0x34	// LoRa WAN public sync word; other: 0x12
#define LPREAMBLE   8		// 8 Bytes
#define IHDMODE     0		// =1 Implcite header Mode, =0 Explicite Header Mode
#define IHEADERLEN	0		// if HEADERMODE = 1 => Lenght of header in Bytes 
#define NOCRC		0		// =1 NOCRC; =0 CRC check
#define RX_TOUT_LSB	0x05	// RX_TOut_LSB = 5 x Tsymbol
#define NORXIQINV	1		// =1 No Invert IQ 

// Basically expected to be defined in lora radio layer header file,
// but we need it common for both sides
#define MAX_PAYLOAD_LENGTH 0x80 // length of LoRa Pkg in IHDMODE=0: BEEIOT_HDRLEN..0xFF 

// Gateway identifier (for destID/sendID)
#define GWID1		0x99	// Transfer ID of this gateway (main)
#define GWID2		0x98	// Transfer ID of this gateway (backup)

// Node identifier (for destID/sendID)
#define NODEID1		0x81	// Transfer ID of LoRa Client 1
#define NODEID2		0x82	// Transfer ID of LoRa Client 2
#define NODEID3		0x83	// Transfer ID of LoRa Client 3
#define NODEID4		0x84	// Transfer ID of LoRa Client 4
#define NODEID5		0x85	// Transfer ID of LoRa Client 5

// BeeIoTWan Pkg Header and Flow control definitions
#define BEEIOT_HDRLEN	5	// current BeeIoT-WAN Package headerLen
#define BEEIOT_DLEN		MAX_PAYLOAD_LENGTH-BEEIOT_HDRLEN
#define BEEIOT_STATUSCNT 14	// number of status parameters to be parsed from user payload
#define	MSGBURSTWAIT	500	// repeat each 0.5 seconds the message 
#define MAXRXACKWAIT	10	// # of Wait loops of MSGBURSTWAIT
#define MSGMAXRETRY		3	// Do it max. n times again
#define RXACKGRACETIME  1000 // in ms: time to wait for sending Ack after RX pkg in BeeIoTParse()

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
	CMD_RES5,		// reserved
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
	[CMD_RES5]		= "NOP5",
	[CMD_RES6]		= "NOP6", 
	[CMD_RES7]		= "NOP7",
	[CMD_RES8]		= "NOP8", 
	[CMD_RES9]		= "NOP9",
	[CMD_NOP]		= "NOP"
};

//***************************************************************
// BeeIoT-WAN common package header format (e.g. for casting on received LoRa MAC payloads)
// BeeIoT LoRa Package == User-Payload[MAX_PAYLOAD_LENGTH] 
typedef struct {
	byte	destID;		// ID of receiver
	byte	sendID;		// ID of BeeIoT Node sending a package
	byte	index;		// package index (0..0xFF)
	byte	cmd;		// command code from Node
	byte	length;		// length of last status data string
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
typedef struct {
	beeiot_header_t hd;	// BeeIoT common Header
	char	djoin[BEEIOT_DLEN]; // remaining status array
} beeiot_join_t;
#define beeiot_nparam_join	1

//***************************
// STATUS-CMD Pkg:
typedef struct {
	beeiot_header_t hd;	// BeeIoT common Header
	char	dstat[BEEIOT_DLEN]; // remaining status array
} beeiot_status_t;
#define beeiot_nparam_status	14

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