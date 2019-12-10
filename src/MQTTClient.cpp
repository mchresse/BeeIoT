#include "MQTTClient.h"

extern uint16_t	lflags;      // BeeIoT log flag field



// Constructor of Class MQTTClient   (function name = class name)
MQTTClient::MQTTClient()
{
	
}


// Send MQTT CONNECT request to MQTT Server via TCP
// if username == "" then username/password will not be integrated in the request
// returns 1 if accepted othwerwise 0
int MQTTClient::Connect(String clientid, String username, String password)
{
    int result = 0;

    // fill the buffer with data to be sent for connection request
    int BufferIndex = 2;  //  we start at index 2 to leave room for message type byte and remaininglength byte

    // for MQTT version 3.1.1 you always have to sent the following bytes
    byte d[7] = { 0, 4, 77, 81, 84, 84, 4};

    // fill the buffer with these bytes
    for (int j = 0 ;j < 7; j++) 
    {
        SendBuffer[BufferIndex++] = d[j];
    }

    // next byte position is filled with flags
    byte ConnectFlags = 0x02;   // set clean session flag
    if (username != "")
    {
      ConnectFlags = 0x80 + 0x40 + 0x02;  // set the flags for username/password
    }
    SendBuffer[BufferIndex++] = ConnectFlags;

    // next two bytes are keep alive time
    SendBuffer[BufferIndex++] = 0;
    SendBuffer[BufferIndex++] = 180;

    // then write the client-ID in the buffer, with length bytes at the beginning
    BufferIndex = WriteStringInBuffer(clientid, SendBuffer, BufferIndex);

    if (username != "")
    {
      // write the username in the buffer, with length bytes at the beginning
      BufferIndex = WriteStringInBuffer(username, SendBuffer, BufferIndex);
  
      // write the password in the buffer, with length bytes at the beginning
      BufferIndex = WriteStringInBuffer(password, SendBuffer, BufferIndex);
    }

    // send the buffer via TCP/IP, put message type and remaining length byte at the beginning
    result = Send(MQTTCONNECT, SendBuffer, BufferIndex - 2); 

    delay(1000);

    // get bytes back from TCPServer = MQTT Broker
    byte ReturnLength = myTCPClient.Receive(ReturnBuffer);
    
    if (ReturnLength > 0)
    {
      if ((ReturnBuffer[0] != 32) || (ReturnBuffer[1] != 2))
      {
        result = 0; // answer of MQTT Broker is not 32, 2, ...
      }      
    }
    else
    {
      result = 0; // no answer of MQTT Broker        
    }
  
    return result;  
}



// Send MQTT-Publish-message to MQTT-Server via TCP
// the byte array payload contains the payload data, plength is the amount of payload bytes
int MQTTClient::Publish(String topic, byte payload[], int plength) 
{
    // fill the buffer with data to be sent for publish message
    int BufferIndex = 2;  //  we start at index 2 to leave room for message type byte and remaininglength byte

    // write the topic in the buffer, with topic length bytes at the beginning and offset = 2
    BufferIndex = WriteStringInBuffer(topic, SendBuffer, BufferIndex);

    // then copy the payload bytes in the buffer
    for (int i = 0; i  < plength; i++)
    {
        SendBuffer[BufferIndex++] = payload[i];
    }

    // send the buffer via TCP/IP, put message type and remaining length byte at the beginning
    return Send(MQTTPUBLISH, SendBuffer, BufferIndex - 2);
}



// Send out a Ping request to MQTT server, this must be done during the Keep alive time (here: 180 s)
// returns 1 if Ping response was received othwerwise 0 
int MQTTClient::Ping()
{
    int result = 1;
    
    // send the buffer (here: empty) via TCP/IP, put message type and remaining length byte at the beginning
    result = Send(MQTTPINGREQ, SendBuffer, 0);

    delay(1000);

    // get bytes back from TCPServer = MQTT Broker
    byte ReturnLength = myTCPClient.Receive(ReturnBuffer);

    if (ReturnLength > 0)
    {
      if ((ReturnBuffer[0] != 208) || (ReturnBuffer[1] != 0))
      {
        result = 0; // answer of MQTT Broker is not 208, 0
      }      
    }
    else
    {
      result = 0; //  no answer of MQTT Broker   
    }

    return result;
    
}



// Send MQTT-Subscribe-message to MQTT-Server via TCP
// returns 1 if accepted othwerwise 0 
int MQTTClient::Subscribe(String topicfilter) 
{
    int result = 1;
    
    // fill the buffer with data to be sent for publish message
    int BufferIndex = 2;  //  we start at index 2 to leave room for message type byte and remaininglength byte

    SendBuffer[BufferIndex++] = 0x00;
    SendBuffer[BufferIndex++] = 0x02;

    // write the topic in the buffer, with topic length bytes at the beginning and offset = 2
    BufferIndex = WriteStringInBuffer(topicfilter, SendBuffer, BufferIndex);

    SendBuffer[BufferIndex++] = 0x00;  // Quality of service

    // send the buffer via TCP/IP, put message type and remaining length byte at the beginning
    result = Send(MQTTSUBSCRIBE + 2, SendBuffer, BufferIndex - 2);
    
    delay(1000);

    // get bytes back from TCPServer = MQTT Broker
    byte ReturnLength = myTCPClient.Receive(ReturnBuffer);

    if (ReturnLength > 0)
    {
      if ((ReturnBuffer[0] != 144) || (ReturnBuffer[1] != 3))
      {
        result = 0; // answer of MQTT Broker is not 144, 3, ...
      }      
    }
    else
    {
      result = 0; //  no answer of MQTT Broker   
    }

    return result;
}



// Polls if MQTT message from server was received (of course topic must match a subscribed topic filter)
// stores the payload in MQTTBytesPayloadBuffer which must be allocated by the calling function
// returns the number of payload bytes received, 0 when no bytes were received

int MQTTClient::Get(byte MQTTBytesPayloadBuffer[])
{   
    byte MQTTBytesReceivedBuffer[128];
    byte MQTTBytesReceivedCount = myTCPClient.Receive(MQTTBytesReceivedBuffer);

    byte MQTTBytesPayloadCount = 0;

    // we received bytes from the broker
    if (MQTTBytesReceivedCount > 0)
    {
      // with a Publish command byte at the beginning
      // so we know this must be a message from some client
      if (MQTTBytesReceivedBuffer[0] == MQTTPUBLISH)
      {
         //byte RemainingLengthByte = MQTTBytesReceivedBuffer[1];
         
         // get the length of the Publish topic inside this message
         // remark: we do not process the topic here
         byte TopicLength = MQTTBytesReceivedBuffer[3];

         // get the payload bytes
         for (int c = 4 + TopicLength; c < MQTTBytesReceivedCount; c++)
         {
            // put it in MQTTBytesPayloadBuffer[]
            MQTTBytesPayloadBuffer[c - 4 - TopicLength] = MQTTBytesReceivedBuffer[c];
         }

         // calculate number of payload bytes
         MQTTBytesPayloadCount = MQTTBytesReceivedCount - 4 - TopicLength;
      }
       
    }

    return MQTTBytesPayloadCount;
}



// send out a MQTT message from client to server
// send the buffer via TCP/IP, put message type and remaining length byte at the beginning
// buf contains the bytes to be sent out, with 2 bytes left free at the beginning
// RemainingLengthBytes is the number of bytes, without the 2 bytes at the beginning
int MQTTClient::Send(byte MessageType, byte buf[], int RemainingLengthByte)
{   
    buf[0] = MessageType;        
    buf[1] = RemainingLengthByte & 255;
      
    // send out all bytes via TCP/IP
    int result = myTCPClient.Send(buf, RemainingLengthByte + 2);

    return result;
}





// this function writes the bytes (ASCII) of a string in the buffer buf, with a given offset, and returns a new offset/index back
int MQTTClient::WriteStringInBuffer(String MString, byte buf[], int offset) 
{
  
    int Slength = MString.length();
    
    // copy the bytes of this array in the buffer, with the given offset, and leave 2 bytes free at the beginning
    for (int j = 0; j < Slength; j++)
    {
        buf[offset + 2 + j] = MString[j];
    }

    // fill the lenght of the string in the first 2 bytes (string length is lower than 128 in that case)
    buf[offset] = 0;
    buf[offset + 1] = Slength;

    // return new offset/index after the string
    return offset + 2 + Slength;
};








