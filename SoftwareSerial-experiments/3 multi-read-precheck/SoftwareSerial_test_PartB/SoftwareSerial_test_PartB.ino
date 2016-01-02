
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

#define NUM_CONN 3 

SoftwareSerial* bus[NUM_CONN];
int RX[NUM_CONN] = {A0, A2, A4};
int TX[NUM_CONN] = {A1, A3, A5};

#define coil1 6
#define coil2 5
#define MAX_COUNTER 1000; //4294967295;
unsigned long counter = MAX_COUNTER;
boolean coilEnabled = false;

volatile uint8_t *_receivePortRegister0;
uint8_t _receiveBitMask0;
volatile uint8_t *_receivePortRegister1;
uint8_t _receiveBitMask1;
volatile uint8_t *_receivePortRegister2;
uint8_t _receiveBitMask2;

void setup(){
  pinMode(coil1, OUTPUT);
  pinMode(coil2, OUTPUT);
  
  // add arguments of false (for inverse_logic?), false (for full-duplex?) for half duplex.
  // you can then use the same pin for both transmit and receive.
  for (int i=0; i<NUM_CONN; i++) {
    bus[i] = new SoftwareSerial(RX[i], TX[i], false, false);
    bus[i]->begin(9600); //1200
  }
  
  Serial.begin(9600);            // to see who's connected
  
  uint8_t port0 = digitalPinToPort(RX[0]);
  _receivePortRegister0 = portInputRegister(port0);
  _receiveBitMask0 = digitalPinToBitMask(RX[0]);
  
  uint8_t port1 = digitalPinToPort(RX[1]);
  _receivePortRegister1 = portInputRegister(port1);
  _receiveBitMask1 = digitalPinToBitMask(RX[1]);
  
  uint8_t port2 = digitalPinToPort(RX[2]);
  _receivePortRegister2 = portInputRegister(port2);
  _receiveBitMask2 = digitalPinToBitMask(RX[2]);
  
  // bus[0]->listen();
}

uint8_t rx_pin_read0() {
  return *_receivePortRegister0 & _receiveBitMask0;
}
uint8_t rx_pin_read1() {
  return *_receivePortRegister1 & _receiveBitMask1;
}
uint8_t rx_pin_read2() {
  return *_receivePortRegister2 & _receiveBitMask2;
}

void loop(){
  
  if (rx_pin_read0()) {
    // bus[0]->listen();
    Serial.println('a');
  }
  if (rx_pin_read1()) {
    // bus[1]->listen();
    Serial.println('b');
  }
  if (rx_pin_read2()) {
    // bus[2]->listen();
    Serial.println('c');
  }
  
  // for (int i=0;i<NUM_CONN;i++) {
  //   if( bus[i]->available() ){
  //     char c = bus[i]->read();
  //     Serial.write(c);
  //   }
  //   if( Serial.available()){
  //     counter = MAX_COUNTER;
  //     int index = int(Serial.read())-48;
  //     bus[i]->write(index);
  //   }
  // }
  // if(counter > 0) {
  //   updateCoil(true);
  // } else {
  //   updateCoil(false);
  // }
  // if(counter > 0) counter --;
}

void updateCoil(boolean enable) {
  if(enable != coilEnabled) {
    digitalWrite(coil1, enable? HIGH : LOW);
    coilEnabled = enable;
  }
}
