#include <SoftwareSerial.h>

#define NUM_CONN 3

SoftwareSerial* bus[NUM_CONN];
int RX[NUM_CONN] = {A1, A3, A5};
int TX[NUM_CONN] = {A0, A2, A4};
int coil[NUM_CONN] = {6, 5, 10};

#define leds1pin 11
#define leds2pin 3

// ToDo: turn into arrays / enums:
volatile uint8_t *_rx0PortRegister;
uint8_t _rx0BitMask;
volatile uint8_t *_rx1PortRegister;
uint8_t _rx1BitMask;
volatile uint8_t *_rx2PortRegister;
uint8_t _rx2BitMask;

const int myID = 1;

int activeBus = -1;

unsigned long listenStartTime = 0;
#define listenTimeout 20

void setup(){
  for (int i=0; i<NUM_CONN; i++) {
    bus[i] = new SoftwareSerial(RX[i], TX[i]);
    bus[i]->begin(9600); //1200
    pinMode(coil[i], OUTPUT);
  }

  pinMode(leds1pin, OUTPUT);
  pinMode(leds2pin, OUTPUT);

  digitalWrite(leds1pin, LOW);
  digitalWrite(leds2pin, LOW);

  uint8_t port0 = digitalPinToPort(RX[0]);
  _rx0PortRegister = portInputRegister(port0);
  _rx0BitMask = digitalPinToBitMask(RX[0]);

  uint8_t port1 = digitalPinToPort(RX[1]);
  _rx1PortRegister = portInputRegister(port1);
  _rx1BitMask = digitalPinToBitMask(RX[1]);

  uint8_t port2 = digitalPinToPort(RX[2]);
  _rx2PortRegister = portInputRegister(port2);
  _rx2BitMask = digitalPinToBitMask(RX[2]);

  Serial.begin(9600);
}

void loop(){

  if (activeBus==-1) {
    if (!rx0Read()) {
      setActiveBus(0);
    }
    if (!rx1Read()) {
      setActiveBus(1);
    }
    if (!rx2Read()) {
      setActiveBus(2);
    }
  }
  else {
    readActiveBus();
  }
}

uint8_t rx0Read() {
  return *_rx0PortRegister & _rx0BitMask;
}
uint8_t rx1Read() {
  return *_rx1PortRegister & _rx1BitMask;
}
uint8_t rx2Read() {
  return *_rx2PortRegister & _rx2BitMask;
}

void setActiveBus(int index) {
  activeBus = index;
  listenStartTime = millis();
  bus[index]->listen();
  // SoftwareSerial::handle_interrupt(); // not possible? So requires sending a pulse before sending message.
}

void readActiveBus() {
  //if( bus[activeBus]->available() ){
  while( bus[activeBus]->available() ){
    // only toggle bus's led when index is myID ?
    int index = int(bus[activeBus]->read());
    if(index != 0) {
      // Serial.write(index);
      Serial.println(index);
      //if( index == myID ){
        // int brightness = index*100/10*255/100;
        int brightness = float(index)/10*255;
        //toggleCoil(activeBus, brightness);
        setCoil(activeBus, brightness);
      //   bus[index]->print("I am ");
      //   bus[index]->println(myID);
      //}
      // since we're only sending one thing, after reading it we start checking other ports again
      activeBus = -1;
      break;
    }
  }
  if (millis()-listenStartTime > listenTimeout) activeBus = -1;
}

void toggleCoil(int index) {
  digitalWrite(coil[index], !digitalRead(coil[index]));
}
void setCoil(int index, int value) {
  analogWrite(coil[index], value);
}
