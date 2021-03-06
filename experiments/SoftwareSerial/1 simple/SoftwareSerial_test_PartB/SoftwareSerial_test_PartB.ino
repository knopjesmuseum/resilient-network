/*
  This example program can be used to test the "SoftwareSerialWithHalfDuplex" library, adapted from the SoftwareSerial library.
  The value of half-duplex is that one pin can be used for both transmitting and receiving data.
  Also many devices can be daisy-chained to the same line. RS485 still commonly uses half-duplex.
  
  By default the library works the same as the SoftwareSerial library, but by adding a couple of additional arguments
  it can be configured for half-duplex. In that case, the transmit pin is set by default to an input, with the pull-up set. 
  When transmitting the pin temporarily switches to an output until the byte is sent, then flips back to input. When a module is 
  receiving it should not be able to transmit, and vice-versa. This library probably won't work as is if you need inverted-logic.
  
  To use this test example, upload SoftwareSerialWithHalfDuplex_test_partA to as many arduinos as you like. Be sure to change 
  "myID" for each arduino loaded with partA. Upload SoftwareSerialWithHalfDuplex_test_partB to a different arduino. All arduinos
  should be connected to each other by the same communications pin, and by ground. Open up the serial monitor pointing to partB.
  When you type in the id number of one of the devices it should respond.
  
  This is a first draft of the library and test programs. It appears to work, but has only been tested on a limited basis,
  and hasn't yet been tested with any native half-duplex devices (like the bioloid ax12 robot servo). 
  Seems fairly reliable up to 57600 baud. As with all serial neither error checking, nor addressing are implemented, 
  so it is likely that you will need to do this yourself. Also, you can make use of other protocols such as i2c. 
  I am looking for any feedback, advice and help at this stage. 
  Contact me at n.stedman@steddyrobots.com or on the arduino forums.
*/



#include <SoftwareSerial.h>

#define rx A1
#define tx A0
#define coil1 6
#define coil2 5
#define leds1pin 11
#define leds2pin 3

#define MAX_COUNTER 1000; //4294967295;
unsigned long counter = MAX_COUNTER;
boolean coilEnabled = false;

// add arguments of false (for inverse_logic?), false (for full-duplex?) for half duplex.
// you can then use the same pin for both transmit and receive.
SoftwareSerial debugPort(rx, tx);

void setup(){
  debugPort.begin(9600);         // to connect with other devices
  Serial.begin(9600);            // to see who's connected
  // pinMode(rx, INPUT_PULLUP);
  
  pinMode(leds1pin, OUTPUT);
  pinMode(leds2pin, OUTPUT);
  digitalWrite(leds1pin, LOW);
  digitalWrite(leds2pin, LOW);
}

void loop(){
  if( debugPort.available() ){
    char c = debugPort.read();
    Serial.write(c);
  }
  if( Serial.available()){
    counter = MAX_COUNTER;
    int i = int(Serial.read())-48;
    debugPort.write(i);
  }
  if(counter > 0) {
    updateCoil(true);
  } else {
    updateCoil(false);
  }
  if(counter > 0) counter --;
}

void updateCoil(boolean enable) {
  if(enable != coilEnabled) {
    digitalWrite(coil1, enable? HIGH : LOW);
    coilEnabled = enable;
  }
}
