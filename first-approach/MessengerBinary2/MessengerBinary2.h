#ifndef Messenger_h
#define Messenger_h

#include "Arduino.h"
#include <SoftwareSerial.h>

typedef void (*MessageHandler)(byte type, byte value,int i);

//#define MAX_TRANSMIT_BUFFER 24
//#define MAX_TRANSMIT_BUFFER 10
#define MAX_TRANSMIT_BUFFER 20
//64
#define MESSAGE_LENGTH 2
#define MESSENGER_DEBUG false
#define MIN_ALIVE_TIME 2000

#define TYPE_LENGTH 2
#define VALUE_LENGTH 3
#define CHECK_LENGTH 3

// predefined message types required for protocol
// message types
#define TYPE_SYSTEM 3
// value types
#define VALUE_LISTENING 0
#define VALUE_ERROR 1

// max number of messages that will be send in one go.
#define MAX_NUM_SEND 5

// max waiting for messages when starting to listing
//#define MAX_INIT_WAITING 100
//#define MAX_INIT_WAITING 40 // nicer
//#define MAX_INIT_WAITING 30 // gives some read errors (not usable for debug)
//#define MAX_INIT_WAITING 2 // fast! (missing lightning message?)
//#define MAX_INIT_WAITING 5 // doesn't solve messages disapering
#define MAX_INIT_WAITING 20 

// max waiting for more messages after receiving a message
//#define MAX_WAITING 30
//#define MAX_WAITING 3 // missing lightning message?
#define MAX_WAITING 20

class MessengerBinary2
{
  public:
	
	int index;
	bool debug;
	bool listening;
	//byte state;

    MessengerBinary2(int i,SoftwareSerial* serial, int txPin, MessageHandler messageHandler, int ledPin);
	MessengerBinary2(int i,SoftwareSerial* serial, int txPin, MessageHandler messageHandler);
    void sendMessage(byte type, byte value, bool overrideSameType);
    void update();
	void listen();
	bool isAlive();

  private:
	
	SoftwareSerial* _serial;
    MessageHandler _messageHandler;
    int _txPin;
	bool _debugLed;
	int _ledPin;
	int _lightCounter;
	
	bool _receivedMessages; // received messages since starting to listen?
	long _lastRxTime; // last time that we received messages
	long _startListeningTime; 

	byte _transmit_buffer[MAX_TRANSMIT_BUFFER][MESSAGE_LENGTH]; 
	int _transmit_buffer_tail;
	int _transmit_buffer_head;
	int _transmit_buffer_prev_head;
	int _prev_transmit_buffer_tail;

	char _typeMask;
	char _valueMask;
	char _checkMask;

	void readMessage(byte message);
	void transmitMessage(byte type, byte value);
};

#endif
