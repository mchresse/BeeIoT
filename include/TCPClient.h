
#ifndef TCPClient_h
#define TCPClient_h

#include <Arduino.h>

#include <WiFi.h>


// define class functions and variables
class TCPClient
{
    // which are accessible by main program
    public:

		// constructor (function name = class name)
		TCPClient();

		// Closes TCP/IP-connection
		// returns 1 when ok, 0 when error
		int Close();

		// Establish TCP/IP-connection to TCP/IP-Server
		// returns 1 when ok, < 0 when error
		int Connect(char* IPAddressString, int Port);

		// Polls if bytes were received via TCP/IP from server
		// stores it in TCPBytesReceivedBuffer which must be allocated by the calling function
		// returns the number of bytes received, 0 when no bytes were received
		int Receive(byte TCPBytesReceivedBuffer[]);

		// send out bytes via TCP/IP to server
		// data contains the bytes to be sent out, len is the number of bytes to be sent out
		// returns 1 when ok, 0 when error
		int Send(byte data[], int len);



};

#endif
