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

// Default frequency plan for EU 868MHz ISM band, based on the Semtech global_conf.json defaults,
// used in The Things Network on Kerlink and other gateways.
// Subject to change!
//
// Bands:
//  g1 :   1%  14dBm  
//  g2 : 0.1%  14dBm  
//  g3 :  10%  27dBm  
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
enum { EU868_FREQ_MIN = 863000000,
       EU868_FREQ_MAX = 870000000 };

enum _cr_t { CR_4_5=0, CR_4_6, CR_4_7, CR_4_8 };
enum _sf_t { SF7=7, SF8, SF9, SF10, SF11, SF12 };
enum _bw_t { BW125=0, BW250, BW500, BW10, BW15, BW20, BW31, BW41, BW62, BWrfu };
enum { RXMODE_SINGLE, RXMODE_SCAN, RXMODE_RSSI };
typedef unsigned char cr_t;
typedef unsigned char sf_t;
typedef unsigned char bw_t;
typedef unsigned char dr_t;


// Queue item for RX packages
typedef struct {
    byte idx;           // index of sent message: 0..255 (round robin)
    byte retries;       // number of initiated retries
	byte ack;			// ack flag 1 = message received
    beeiotpkg_t * data; // sent message struct
} beeiotmsg_t;

void configLoraModem (void);

#endif /* BEELORA_H */