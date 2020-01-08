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
#define MSGBURSTRETRY	1	// Do it 3 times the same.

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
typedef struct {
	byte	destID;		// last ID of receiver/target (GW)
	byte	sendID;		// last ID of BeeIoT Node who has sent the stream
	byte	index;		// last package index
	byte	cmd;		// command code from Node
	byte	length;		// length of last status data string
	char	status[MAX_PAYLOAD_LENGTH-5+1]; // remaining status array
} beeiotdata_t;

typedef struct {
    byte idx;           // index of sent message: 0..255 (round robin)
    byte retries;       // number of initiated retries
	byte ack;			// ack flag 1 = message received
    beeiotdata_t * data; // sent message struct
} beeiotmsg_t;

int LoRaLog(byte cmd, String outgoing);
