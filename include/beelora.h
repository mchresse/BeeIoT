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
#ifndef BEELoRa_H
#define BEELoRa_H

typedef unsigned char      bit_t;
typedef unsigned char	   byte;
typedef unsigned char      u1_t;
typedef   signed char      s1_t;
typedef unsigned short     u2_t;
typedef          short     s2_t;
typedef unsigned int       u4_t;
typedef          int       s4_t;
// typedef unsigned long long u8_t; // already defined in arch.h
// typedef          long long s8_t;
typedef unsigned int       uint;
typedef const char*		   str_t;
typedef              u1_t* xref2u1_t;
typedef        const u1_t* xref2cu1_t;

typedef				  u4_t devaddr_t;		// Device address

enum { RXMODE_SINGLE, RXMODE_SCAN, RXMODE_RSSI };



// Msg Buffer for TX package
typedef struct {
    byte idx;           // index of sent message: 0..255 (round robin)
    byte retries;       // number of initiated retries
	byte ack;			// ack flag 1 = message received
    beeiotpkg_t * pkg;  // real TX pkg struct
} beeiotmsg_t;


void configLoraModem (void);

#endif /* BEELORA_H */