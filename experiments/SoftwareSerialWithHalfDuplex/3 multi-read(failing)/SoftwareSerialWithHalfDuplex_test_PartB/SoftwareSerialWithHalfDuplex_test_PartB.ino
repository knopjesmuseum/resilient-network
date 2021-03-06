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



#include <SoftwareSerialWithHalfDuplex.h>

SoftwareSerialWithHalfDuplex* bus[3];
int RX[3] = {A0, A2, A4};
int TX[3] = {A1, A3, A5};

#define coil1 6
#define coil2 5
#define MAX_COUNTER 1000; //4294967295;
unsigned long counter = MAX_COUNTER;
boolean coilEnabled = false;

void setup(){
  pinMode(coil1, OUTPUT);
  pinMode(coil2, OUTPUT);
  
  // add arguments of false (for inverse_logic?), false (for full-duplex?) for half duplex.
  // you can then use the same pin for both transmit and receive.
  for (int i=0; i<3; i++) bus[i] = new SoftwareSerialWithHalfDuplex(RX[i], TX[i], false, false);
  for (int i=0; i<3; i++) bus[i]->begin(9600); //1200
  
  Serial.begin(9600);            // to see who's connected
}

void loop(){
  for (int i=0;i<3;i++) {
    bus[i]->listen();
    if( bus[i]->available() ){
      char c = bus[i]->read();
      Serial.write(c);
    }
    if( Serial.available()){
      counter = MAX_COUNTER;
      int index = int(Serial.read())-48;
      bus[i]->write(index);
    }
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
