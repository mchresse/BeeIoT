#include "TCPClient.h"

extern uint16_t	lflags;      // BeeIoT log flag field

// TCP Library
// as ESP32 Arduino Version


WiFiClient client;

// Constructor of Class TCPClient   (function name = class name)
TCPClient::TCPClient()
{
	
}



// Closes TCP/IP-connection
// returns 1 when ok, 0 when error

int TCPClient::Close()
{
    int Result = 0;

    client.stop();

    // no detection of error in this implementation of function
    Result = 1;   
    
    return Result;
}




// Establish TCP/IP-connection to TCP/IP-Server
// returns 1 when ok, < 0 when error

int TCPClient::Connect(char* IPAddressString, int Port)
{
    // see https://www.arduino.cc/en/Reference/ClientConnect
    // Returns an int (1,-1,-2,-3,-4) indicating connection status :
    // SUCCESS 1
    // TIMED_OUT -1
    // INVALID_SERVER -2
    // TRUNCATED -3
    // INVALID_RESPONSE -4
    
    int Result = client.connect(IPAddressString, Port);

    return Result;
}




// Polls if bytes were received via TCP/IP from server
// stores it in TCPBytesReceivedBuffer which must be allocated by the calling function
// returns the number of bytes received, 0 when no bytes were received

int TCPClient::Receive(byte TCPBytesReceivedBuffer[])
{
    // see https://www.arduino.cc/en/Reference/StreamReadBytes
    // readBytes() read characters from a stream into a buffer. 
    // The function terminates if the determined length has been read, or it times out (see setTimeout()).

    // here we read 120 Bytes max.
    // Timeout is 1000 ms by default
    int TCPBytesReceivedCount = client.readBytes(TCPBytesReceivedBuffer, 120);

    // number of bytes received from server
    return TCPBytesReceivedCount;
}



// send out bytes via TCP/IP to server
// data contains the bytes to be sent out, len is the number of bytes to be sent out
// returns 1 when ok, 0 when error

int TCPClient::Send(byte data[], int len)
{
     int Result = 0;

     // see https://www.arduino.cc/en/Reference/ClientWrite
     // write() returns the number of bytes written. It is not necessary to read this value.
     int BytesWritten = client.write(data,len);

     // if all bytes were sent out, we assume everything is ok
     if (BytesWritten == len)
     {     
      Result = 1;   
     }

     return Result;
};


