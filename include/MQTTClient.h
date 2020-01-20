// MQTT Library function
// Code based on PubSubClient by Nick O'Leary: http://pubsubclient.knolleary.net/index.html
// Distributed under MIT-License: https://opensource.org/licenses/mit-license.php
// For BeeIoT project used 3rd party open source see also Readme_OpenSource.txt

#ifndef MQTTCLIENT_H
#define MQTTCLIENT_H

#include <Arduino.h>
#include <TCPClient.h>

// define class functions and variables
class MQTTClient
{
    // which are accessible by main program
    public:

		// buffer to store payload bytes
		byte PayloadBuffer[128];
		
		// the TCP Client object for TCP/IP functions
		TCPClient myTCPClient;

		// constructor (function name = class name)
		MQTTClient();

		// Send MQTT CONNECT request to MQTT Server via TCP
		// if username == "" then username/password will not be integrated in the request
		// returns 1 if accepted othwerwise 0
		int Connect(String clientid, String username, String password);


		// Send MQTT-Publish-message to MQTT-Server via TCP
		// the byte array payload contains the payload data, plength is the amount of payload bytes
		int Publish(String topic, byte payload[], int plength);


		// Send out a Ping request to MQTT server, this must be done during the Keep alive time (here: 180 s)
		// returns 1 if Ping response was received othwerwise 0 
		int Ping();


		// Send MQTT-Subscribe-message to MQTT-Server via TCP
		// returns 1 if accepted othwerwise 0 
		int Subscribe(String topicfilter);


		// Polls if MQTT message from server was received (of course topic must match a subscribed topic filter)
		// stores the payload in MQTTBytesPayloadBuffer which must be allocated by the calling function
		// returns the number of payload bytes received, 0 when no bytes were received
		int Get(byte MQTTBytesPayloadBuffer[]);
		
		

	private:
	
	    byte MQTTCONNECT = 16;
		byte MQTTPUBLISH = 48;
		byte MQTTSUBSCRIBE = 128;
		byte MQTTPINGREQ = 192;
		
		

	
	    byte SendBuffer[128];
		byte ReturnBuffer[128];

		int WriteStringInBuffer(String MString, byte buf[], int offset);
		int Send(byte MessageType, byte buf[], int RemainingLengthByte);


};
#endif // MQTTCLIENT_H