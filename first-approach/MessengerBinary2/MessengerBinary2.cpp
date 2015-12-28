#include "Arduino.h"
#include "MessengerBinary2.h"
#include <SoftwareSerial.h>

MessengerBinary2::MessengerBinary2(int i,SoftwareSerial* serial, int txPin, MessageHandler messageHandler)
{
	MessengerBinary2(i,serial,txPin,messageHandler,-1);
}
MessengerBinary2::MessengerBinary2(int i,SoftwareSerial* serial, int txPin, MessageHandler messageHandler, int ledPin)
{
	index = i;
	debug = false;//false;
	listening = false;
	_receivedMessages = false;

	_serial = serial;
	_messageHandler = messageHandler;
	_txPin = txPin;
	_ledPin = ledPin;

	// binary mask to extrude the message parts from an incomming message. 
	_typeMask = round(pow(2,TYPE_LENGTH)-1);
	_valueMask = round(pow(2,VALUE_LENGTH)-1);
	_checkMask = round(pow(2,CHECK_LENGTH)-1);

	pinMode(txPin,INPUT); // Setting TX to input so that it doesn't block RX //TEMP
	pinMode(ledPin,OUTPUT);
	
	#if MESSENGER_DEBUG
	if(debug)
	{
		Serial.begin(9600);
		Serial.println(millis());
	}
	#endif
	_serial->begin(9600);
	_lightCounter = 0;
	
	_prev_transmit_buffer_tail = _transmit_buffer_head = _transmit_buffer_tail = _transmit_buffer_prev_head = 0;
	
	_debugLed = ledPin;
}
void MessengerBinary2::sendMessage(byte type, byte value, bool overrideSameType) 
{	
	#if MESSENGER_DEBUG
	if(debug)
	{
		Serial.print(index);
		Serial.print(" S ");
		Serial.print(type);
		Serial.print(",");
		Serial.println(value,DEC);
	}
	#endif


	if(type == TYPE_SYSTEM && value == VALUE_ERROR)
	{
		// errors are added in front of the queue
		int temp_transmit_buffer_tail = _transmit_buffer_tail-1;
		if(temp_transmit_buffer_tail < 0) temp_transmit_buffer_tail = MAX_TRANSMIT_BUFFER-1;
		
		if (temp_transmit_buffer_tail == _transmit_buffer_head)
		{
			// when the buffer is full I'm overriding messages
			temp_transmit_buffer_tail = _transmit_buffer_tail;
			Serial.println("ot"); // buffer overflow
		}
		_transmit_buffer[temp_transmit_buffer_tail][0] = type; 
		_transmit_buffer[temp_transmit_buffer_tail][1] = value;
		_transmit_buffer_tail = temp_transmit_buffer_tail;
	}
	else
	{
		// when overrideSameType is enabled it will override the value of the message in front of the queue, 
		//   but only when it's the same type
		/*#if MESSENGER_DEBUG
		if(debug)
		{
			Serial.print("  ");
			Serial.print(overrideSameType);
			Serial.print(" ");
			Serial.print(_transmit_buffer_prev_head);
			Serial.print(" ");
			Serial.print(_transmit_buffer[_transmit_buffer_prev_head][0]);
			Serial.print(",");
			Serial.print(_transmit_buffer[_transmit_buffer_prev_head][1]);
			Serial.print(" ");
			Serial.print(_transmit_buffer_tail);
			Serial.print(",");
			Serial.println(_transmit_buffer_head);
		}
		#endif*/

		if(overrideSameType && _transmit_buffer[_transmit_buffer_prev_head][0] == type)
		{
			_transmit_buffer[_transmit_buffer_prev_head][1] = value;

			if(_transmit_buffer_tail == _transmit_buffer_head)
			{
				if(_transmit_buffer_tail == 0) _transmit_buffer_tail = MAX_TRANSMIT_BUFFER-1;
				else _transmit_buffer_tail--;
			}

			/*#if MESSENGER_DEBUG
			if(debug)
			{
				Serial.print("    o ");
				Serial.print(_transmit_buffer[_transmit_buffer_prev_head][0]);
				Serial.print(",");
				Serial.print(_transmit_buffer[_transmit_buffer_prev_head][1]);
				Serial.print(" ");
				Serial.print(_transmit_buffer_tail);
				Serial.print(",");
				Serial.println(_transmit_buffer_head);
			}
			#endif*/
		}
		else
		{
			// other types are added to the back of the queue
			if ((_transmit_buffer_head + 1) % MAX_TRANSMIT_BUFFER != _transmit_buffer_tail) 
			{
				// save new message in buffer: tail points to where byte goes
				_transmit_buffer[_transmit_buffer_head][0] = type; 
				_transmit_buffer[_transmit_buffer_head][1] = value;
				_transmit_buffer_prev_head = _transmit_buffer_head;
				_transmit_buffer_head = (_transmit_buffer_head + 1) % MAX_TRANSMIT_BUFFER;
			} 
			else 
			{
				if(debug) Serial.println("o"); // buffer overflow
			}
		}
	}
}
void MessengerBinary2::update()
{
	// read messages
	while(_serial->available() > 0)
		readMessage(_serial->read());
	
	if(listening && 
		((_receivedMessages && _lastRxTime+millis() > MAX_WAITING) || 
		(!_receivedMessages && _startListeningTime+millis() > MAX_INIT_WAITING)))
		listening = false;

	if(_debugLed != -1)
	{
		_lightCounter--;
		if(_lightCounter < 0) _lightCounter = 0; 
		digitalWrite(_ledPin, ((_lightCounter > 0)? LOW : HIGH));
	}
}
void MessengerBinary2::listen()
{
	listening = true;
	_receivedMessages = false;
	_startListeningTime = millis();
	transmitMessage(TYPE_SYSTEM,VALUE_LISTENING);
	_serial->listen();
}
void MessengerBinary2::readMessage(byte message) 
{
	#if MESSENGER_DEBUG
	if(debug)
	{
		Serial.print(millis()-_lastRxTime);
		Serial.print(" R ");
		Serial.print(message,BIN);
	  	Serial.print(" > ");
	}
	#endif

	// extract message parts
	byte checksum = message & _checkMask;
	message >>= CHECK_LENGTH;
	byte value = message & _valueMask;
	message >>= VALUE_LENGTH;
	byte type = message & _typeMask;

	_lastRxTime = millis();

	if ((type+value)/2 == checksum)
	{
		#if MESSENGER_DEBUG
		if(debug)
		{
			if(type == TYPE_SYSTEM && value == VALUE_LISTENING)
			{
				Serial.println("l");
			}
			else if(type == TYPE_SYSTEM && value == VALUE_ERROR)
			{
				Serial.println("e");
			}
			else
			{
			  	Serial.print(type, DEC);
				Serial.print(",");
				Serial.println(value, DEC);
				//Serial.print(",");
				//Serial.println(checksum, DEC);
			}
		}
		#endif

		if(type == TYPE_SYSTEM)
		{
			if(value == VALUE_LISTENING)
			{
				// send messages
				byte numSend = 0;
				_prev_transmit_buffer_tail = _transmit_buffer_tail;

				#if MESSENGER_DEBUG
				if(debug)
				{
					Serial.print("  ");
					Serial.print(_transmit_buffer_tail);
					Serial.print(" ");
					Serial.println(_transmit_buffer_head);
				}
				#endif

				// if under max send messages and buffer not empty?
				while(numSend < MAX_NUM_SEND && _transmit_buffer_tail != _transmit_buffer_head)
				{
					byte* data = _transmit_buffer[_transmit_buffer_tail]; // grab next byte
					transmitMessage(data[0],data[1]);
					// remove message from buffer / queue
					_transmit_buffer_tail = (_transmit_buffer_tail + 1) % MAX_TRANSMIT_BUFFER;	
					numSend++;
				}
				listening = false;
			}
			else if(value == VALUE_ERROR)
			{
				// reset the tail so that the previous batch of send messages is send again
				_transmit_buffer_tail = _prev_transmit_buffer_tail;
			}
		}
		else
		{
			// inform listener
			(*_messageHandler)(type, value, index);
		}
	}
	else 
	{
		Serial.println("e"); // read error
		sendMessage(TYPE_SYSTEM,VALUE_ERROR,false);
	}
}
void MessengerBinary2::transmitMessage(byte type, byte value)
{
	if(type != TYPE_SYSTEM) _lightCounter = 50;

	//int sumData = (data[0]+data[1])/2;
	//byte checksum = (sumData/256) ^ (sumData&0xFF);
	byte checksum = (type+value)/2; 

	pinMode(_txPin,OUTPUT); // enable TX / output 

	byte output = type; // type
	output <<= VALUE_LENGTH;
	output += value; //value
	output <<= CHECK_LENGTH;
	output += checksum;

	_serial->write(output); // transmit message

	pinMode(_txPin,INPUT); // disable TX / output 
	
	#if MESSENGER_DEBUG
	if(debug)
	{
		Serial.print(index);
		Serial.print("   T ");
		if(type != TYPE_SYSTEM)
		{
			Serial.print(type);
			Serial.print(" ");
			Serial.println(value);
			//Serial.print(" ");
			//Serial.print(checksum);
			//Serial.print(" ");
			//Serial.println(output,BIN);
		}
		else
		{
			if(value == VALUE_LISTENING)
				Serial.println("l");
			else if(value == VALUE_ERROR)
				Serial.println("e");
		}
	}
	#endif
}
bool MessengerBinary2::isAlive()
{
	// when a connection didn't receive a message for longer it's probably not connected
	return (millis()-_lastRxTime < MIN_ALIVE_TIME);
}