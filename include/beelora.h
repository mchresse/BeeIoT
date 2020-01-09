//*******************************************************************
// BeeLora.h  
// from Project https://github.com/mchresse/BeeIoT
//
// Description:
// BeeIoT-WAN - Lora package flow definitions
//
//----------------------------------------------------------
// Copyright (c) 2019-present, Randolph Esser
// All rights reserved.
// This file is distributed under the BSD-3-Clause License
// The complete license agreement can be obtained at: 
//     https://github.com/mchresse/BeeIoT/license
// For used 3rd party open source see also Readme_OpenSource.txt
//*******************************************************************

#define BEEIOT_DEVID    0x77
#define BEEIOT_GWID     0x88

#define MAX_PAYLOAD_LENGTH  0x80  // max length of LoRa MAC raw package
#define	MSGBURSTWAIT	500	// repeat each 0.5 seconds the message 
#define MAXRXACKWAIT	10	// # of Wait loops of MSGBURSTWAIT
#define MSGMAXRETRY		3	// Do it max. n times again
#define MAXRXPKG		3	// Max. number of parallel processed RX packages

// LoRa "SendMessage()" user command codes
#define CMD_NOP			0	// do nothing -> for xfer test purpose
#define CMD_LOGSTATUS	1	// process Sensor log data set
#define CMD_SDLOG		2	// one more line of SD card log file
#define CMD_RETRY		3	// Tell target: Package corrupt, do it again
#define CMD_ACK			4	// Received message complete
#define CMD_NOP5		5	// reserved
#define CMD_NOP6		6	// reserved
#define CMD_NOP7		7	// reserved

// LoRa raw data package format (e.g. for casting on received LoRa MAC payload)
#define BEEIOT_HDRLEN	5	// current BeeIoT-WAN Package headerLen
#define BEEIOT_DLEN		MAX_PAYLOAD_LENGTH-BEEIOT_HDRLEN
typedef struct {
	byte	destID;		// ID of receiver/target (GW)
	byte	sendID;		// ID of BeeIoT Node who has sent the stream
	byte	index;		// package index
	byte	cmd;		// command code from Node
	byte	length;		// length of following status data string (0 .. MAX_PAYLOAD_LENGTH-5 incl. EOL(0D0A))
	char	data[BEEIOT_DLEN]; // remaining status array excl. this header bytes above
} beeiotpkg_t;

typedef struct {
    byte idx;           // index of sent message: 0..255 (round robin)
    byte retries;       // number of initiated retries
	byte ack;			// ack flag 1 = message received
    beeiotpkg_t * data; // sent message struct
} beeiotmsg_t;

int  BeeIoTParse	(beeiotpkg_t * msg);
int  sendMessage	(beeiotpkg_t * TXData, int sync);
void onReceive		(int packetSize);
