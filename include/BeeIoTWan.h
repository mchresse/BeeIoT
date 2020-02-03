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

#define BIoT_VERSION	1000		// 2By: starting with V1.0.00

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
enum _dr_eu868_t { DR_SF12=0, DR_SF11, DR_SF10, DR_SF9, DR_SF8, DR_SF7, DR_SF7B, DR_FSK, DR_NONE };

typedef unsigned int  devaddr_t;		// Node dynmic Device address (given by GW)
typedef unsigned char cr_t;
typedef unsigned char sf_t;
typedef unsigned char bw_t;
typedef unsigned char dr_t;

// Radio parameter set (encodes SF/BW/CR/IH/NOCRC)
typedef unsigned short rps_t;			
inline sf_t  getSf   (rps_t params)            { return   (sf_t)(params &  0x7); }
inline rps_t setSf   (rps_t params, sf_t sf)   { return (rps_t)((params & ~0x7) | sf); }
inline bw_t  getBw   (rps_t params)            { return  (bw_t)((params >> 3) & 0x3); }
inline rps_t setBw   (rps_t params, bw_t cr)   { return (rps_t)((params & ~0x18) | (cr<<3)); }
inline cr_t  getCr   (rps_t params)            { return  (cr_t)((params >> 5) & 0x3); }
inline rps_t setCr   (rps_t params, cr_t cr)   { return (rps_t)((params & ~0x60) | (cr<<5)); }
inline int   getNocrc(rps_t params)            { return        ((params >> 7) & 0x1); }
inline rps_t setNocrc(rps_t params, int nocrc) { return (rps_t)((params & ~0x80) | (nocrc<<7)); }
inline int   getIh   (rps_t params)            { return        ((params >> 8) & 0xFF); }
inline rps_t setIh   (rps_t params, int ih)    { return (rps_t)((params & ~0xFF00) | (ih<<8)); }
inline rps_t makeRps (sf_t sf, bw_t bw, cr_t cr, int ih, int nocrc) {
    	return sf | (bw<<3) | (cr<<5) | (nocrc?(1<<7):0) | ((ih&0xFF)<<8);
	}
#define MAKERPS(sf,bw,cr,ih,nocrc) ((rps_t)((sf) | ((bw)<<3) | ((cr)<<5) | ((nocrc)?(1<<7):0) | ((ih&0xFF)<<8)))
#define RPSTMPL	"SFxBWxxxCRxIHxNOCRCx"	// string base alternative to rps_t
#define RPSTMPLLEN 20	// is 20 byte long -> used for biot_cfg_t ?

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
#define GWID1		0x99	// Transfer ID of this gateway (default for joining & main RX/TX session)
#define GWID2		0x98	// Transfer ID of this gateway (backup srv., SDLog, FW upd.)

// Node identifier (for destID/sendID)
#define NODEID1		0x81	// Transfer ID of LoRa Client 1
#define NODEID2		0x82	// Transfer ID of LoRa Client 2
#define NODEID3		0x83	// Transfer ID of LoRa Client 3
#define NODEID4		0x84	// Transfer ID of LoRa Client 4
#define NODEID5		0x85	// Transfer ID of LoRa Client 5

// Basically expected to be defined in lora radio layer header file,
// but we need it common for both sides
#define MAX_PAYLOAD_LENGTH 0x80 // length of LoRa Pkg in IHDMODE=0: BEEIOT_HDRLEN..0xFF 

// BeeIoTWan Pkg Header and Flow control definitions
#define BIoT_HDRLEN	5	// current BeeIoT-WAN Package headerLen
#define BIoT_DLEN		MAX_PAYLOAD_LENGTH-BIoT_HDRLEN
#define BIoT_MICLEN     4	// length of NsMIC field
#define BIoT_FRAMELEN	BIoT_DLEN-BIoT_MICLEN	// length of BIoT App-Frame (encoded by AppKey) - MIC
#define	MSGRESWAIT		500	// scan each 0.5 seconds the message status in a waitloop
#define MAXRXACKWAIT	6	// # of Wait loops of MSGRESWAIT
#define MSGMAXRETRY		5	// Do it max. n times again
#define RXACKGRACETIME  1000 // in ms: time to wait for sending Ack after RX pkg in BeeIoTParse()
#define WAITRX1PKG		5	// # of sec. to wait for add. RX pkg after last ACK

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
	CMD_TIME,		// Request curr. time values from partner side 
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
	[CMD_TIME]		= "GETTIME",
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
// JOIN-CMD Pkg:
#define beeiot_nparam_join	1
enum { // Join Request frame format
    OFF_JR_JOINEUI  = 0,
    OFF_JR_DEVEUI   = 8,
    OFF_JR_FRMID 	= 16,
	OFF_JR_VERMAJ	= 18,
	OFF_JR_VERMIN	= 19,
    OFF_JR_MIC      = 20,
};
enum {
    // Join Accept frame format: CONFIG
    OFF_JA_HDR      = 0,
    OFF_JA_ARTNONCE = 1,
    OFF_JA_NETID    = 4,
    OFF_JA_DEVADDR  = 7,
    OFF_JA_RFU      = 11,
    OFF_JA_DLSET    = 11,
    OFF_JA_RXDLY    = 12,
    OFF_CFLIST      = 13,
    LEN_JA          = 17,
    LEN_JAEXT       = 17+16
};
typedef struct {
	byte	joinEUI[8];	// ==AppEUI to address the right App service behind GW+NWserver
	byte	devEUI[8];	// could be e.g. 0xFFFE + node board ID (extended from 6 -> 8Byte)
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
#define beeiot_nparam_cfg	1
typedef struct {
	// device descriptor
	byte	gwid;			// LoRa Pkg target GWID of serving gateway for Status reports
	byte	nodeid;			// LoRa modem unique NodeID1..n used for each Pkg
	byte	version;		// version of used/comitted BeeIoTWAN protocol
	byte	freqsensor;		// =1..n Sensor report frequency in [minutes]
	byte	verbose;		// =0..0xff verbose flags for debugging
	// LoRa Modem settings
	long	lorafreq;		// =EU868_F1..9,DN (EU868_F1: 868.1MHz)
	sf_t	lorasf;			// =0..8 Spreading factor FSK,7..12,SFrFu (1:SF7)
	bw_t	lorabw;			// =0..3 RFU Bandwidth 125-500 (0:125kHz)
	cr_t	loracr;			// =0..3 Coding mode 4/5..4/8 (0:4/5)

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
typedef struct {
	beeiot_header_t hd;			// BeeIoT common Header
	char	dstat[BIoT_FRAMELEN]; 	// remaining status array
} beeiot_status_t;
#define BEEIOT_STATUSCNT 14		// number of status parameters to be parsed from user payload

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
#define BEEIOT_SDNPAR	1	// one big binary data package
#define BEEIOT_MAXDSD		BEEIOT_DLEN-1	// length of raw data 
#define BEEIOT_maxsdlen		

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
typedef struct {
unsigned int frq;
bw_t	band;
dr_t	drmap;
sf_t	sf;
cr_t 	cr;
byte	pwr;
byte	dc;		// duty cycle quote
}channeltable_t;
/*
channeltable_t	txchntab[MAX_CHANNELS] ={		// ChannelIdx:
//	Frequ.		BW		DR	 SFx	CR	  Pwr  DC
	EU868_F1, BW125, DR_SF7, SF7, CR_4_5, 14,  10,	// 0 For Join default: SF7 ... SF10
	EU868_F1, BW250, DR_SF7, SF7, CR_4_5, 14,  10,	// 1 	dito
	EU868_F1, BW500, DR_SF7, SF7, CR_4_5, 14,  10,	// 2 	dito
	EU868_F2, BW125, DR_SF7, SF7, CR_4_5, 14,  10,	// 3
	EU868_F2, BW250, DR_SF7, SF7, CR_4_5, 14,  10,	// 4
	EU868_F2, BW500, DR_SF7, SF7, CR_4_5, 14,  10,	// 5
	EU868_F3, BW125, DR_SF7, SF7, CR_4_5, 14,  10,	// 6
	EU868_F3, BW250, DR_SF7, SF7, CR_4_5, 14,  10,	// 7
	EU868_F3, BW500, DR_SF7, SF7, CR_4_5, 14,  10,	// 8
	EU868_F4, BW125, DR_SF7, SF7, CR_4_5, 14,   1,	// 9
	EU868_F4, BW250, DR_SF7, SF7, CR_4_5, 14,   1,	// 10
	EU868_F4, BW500, DR_SF7, SF7, CR_4_5, 14,   1,	// 11
	EU868_F5, BW125, DR_SF7, SF7, CR_4_5, 14,   1,	// 12
	EU868_F5, BW250, DR_SF7, SF7, CR_4_5, 14,   1,	// 13
	EU868_F5, BW500, DR_SF7, SF7, CR_4_5, 14,   1,	// 14
	EU868_DN, BW125, DR_SF7, SF7, CR_4_5, 27, 100	// 15 For Downlink
};
*/
#endif /* BEEIOTWAN_H */