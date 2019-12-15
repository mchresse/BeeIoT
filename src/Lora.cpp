//*******************************************************************
// LoRa Core layer - Support routines
//*******************************************************************
// For ESP32-DevKitC PIN Configuration look at BeeIoT.h

//*******************************************************************
// LoRa local Libraries
//*******************************************************************

#include <Arduino.h>
#include <stdio.h>
#include "beeiot.h"     // provides all GPIO PIN configurations of all sensor Ports !
#include <LoRa.h>

//***********************************************
// Global Sensor Data Objects
//***********************************************
extern uint16_t	lflags;      // BeeIoT log flag field
int     islora;              // Semaphor of active LoRa Node (=0: active)

const int csPin     = BEE_CS;     // LoRa radio chip select
const int resetPin  = BEE_RST;    // LoRa radio reset
const int irqPin    = BEE_DIO0;   // change for your board; must be a hardware interrupt pin

// String outgoing;              // outgoing message
byte msgCount = 0;            // count of outgoing messages
byte localAddress = 0x77;     // address/ID of this device
byte destination  = 0x88;     // destination ID to send to

#define LORA_SF   7           // preset spreading factor
#define LoRaWANSW 0xf3	      // LoRa WAN public sync word; def: 0x34
#define SIGNALBW  0x125E3     // default Signal bandwidth
#define LORA_FREQ 8681E5      // Band: 868 MHz
#define LORA_PWR  14          // Power Level: 14dB
#define PAYLOAD_LENGTH       0x40

//*********************************************************************
// Setup_LoRa(): init BEE-client object: LoRa
//*********************************************************************
int setup_LoRa(){
    BHLOG(LOGLORA) Serial.printf("  LoRa: Start Lora SPI Init\n");
    islora = -1;

 if (!LoRa.begin(LORA_FREQ)) {          // initialize ratio at LORA_FREQ [MHz] 
                                        // for SX1276 only (Version = 0x12)
    Serial.println("  LoRa: SX1276 not detected. Check your GPIO connections.");
    return(islora);                          // No SX1276 module found -> Stop LoRa protocol
  }

// Presetting of all LoRa protocol parameters if apart from the class default
  LoRa.setSpreadingFactor(LORA_SF);     // ranges from 6-12,default 7 see API docs
  LoRa.setSyncWord(LoRaWANSW);          // ranges from 0-0xFF, default 0x34, see API docs
  LoRa.setTxPower(LORA_PWR);            // no boost mode, outputPin = default
  LoRa.setSignalBandwidth(SIGNALBW);    // set Signal Bandwidth 
//  LoRa.dumpRegisters(Serial);         // dump all SX1276 registers in 0xyy format for debugging

  LoRa.onReceive(onReceive);            // set callback function
  LoRa.receive();                       // start flow control
  BHLOG(LOGLORA) Serial.println("  LoRa: entered receive mode");

  msgCount = 0;                         // no package sent by now
  islora=0;                             // -> LORA is active
  BHLOG(LOGLORA) Serial.println("  LoRa: init succeeded.");

  return(islora);
} // enfd of Setup_LoRa()


void sendMessage(String outgoing) {
int length;

  if(LoRa.beginPacket() ==0)  // start package creation
    return;                   // still transmitting -> come back later

  length = (int) outgoing.length();
  if(length > PAYLOAD_LENGTH)
    length=PAYLOAD_LENGTH;         // limit payload length

  LoRa.write(destination);              // add destination address
  LoRa.write(localAddress);             // add sender address
  LoRa.write(msgCount);                 // add message ID
  LoRa.write(length);                   // limit payload length
  LoRa.print(outgoing);                 // add payload
  LoRa.endPacket();                     // finish packet and send it

  BHLOG(LOGLORA) Serial.printf("  LoRa[0x%x > 0x%x]: #%i:<%s> to 0x%x - %i bytes\n", 
        localAddress, destination, msgCount, outgoing.c_str(), destination, length);
  BHLOG(LOGLORA) hexdump((unsigned char*) & outgoing[0], outgoing.length());

  msgCount++;                           // increment message ID
}

// IRQ callback function -> package received
void onReceive(int packetSize) {
  if (packetSize == 0) return;          // if there's no packet, return

  // read packet header bytes:
  int recipient = LoRa.read();          // recipient address
  byte sender = LoRa.read();            // sender address
  byte incomingMsgId = LoRa.read();     // incoming msg ID
  byte incomingLength = LoRa.read();    // incoming msg length

  String incoming = "";                 // payload of packet

  while (LoRa.available()) {            // can't use readString() in callback, so
    incoming += (char)LoRa.read();      // add bytes one by one
  }
  BHLOG(LOGLORA) Serial.printf("  LoRa: onReceive(): Got a message: Recipient:%i, Sender:%i, MsgID: %i, len:%i\n",
      recipient, sender, incomingMsgId, incomingLength);

  if (incomingLength != incoming.length()) {   // check length for error
    Serial.println("  LoRa: error: IN-message length does not match length in pkg-header");
    return;                             // skip rest of function
  }

  // if the recipient isn't this device or broadcast,
  if (recipient != localAddress && recipient != 0xFF) {
    Serial.println("  LoRa: This message is not for me.");
    return;                             // skip rest of function
  }

  // if message is for this device, or broadcast, print details:
  BHLOG(LOGLORA) Serial.println("    Received from: 0x" + String(sender, HEX));
  BHLOG(LOGLORA) Serial.println("    Sent to      : 0x" + String(recipient, HEX));
  BHLOG(LOGLORA) Serial.println("    Message ID   : " + String(incomingMsgId));
  BHLOG(LOGLORA) Serial.println("    Messagelength: " + String(incomingLength));
  BHLOG(LOGLORA) Serial.println("    RSSI         : " + String(LoRa.packetRssi()));
  BHLOG(LOGLORA) Serial.println("    SNR          : " + String(LoRa.packetSnr()));
  BHLOG(LOGLORA) Serial.println("    Message      : " + incoming);
  BHLOG(LOGLORA) Serial.println();
}

//******************************************************************************
// print hexdump of array  in the format:
// 0xaaaa:  dddd dddd dddd dddd  dddd dddd dddd dddd  <cccccccc cccccccc>
void hexdump(unsigned char * msg, int len){
	int i, y, count;
	unsigned char c;

  Serial.printf("MSGfield at 0x%X:\n", (unsigned int) msg);
	Serial.printf("Address:  0 1  2 3  4 5  6 7   8 9  A B  C D  E F  lenght=%iByte\n", len);
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