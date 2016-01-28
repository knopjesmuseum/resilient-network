#include <SoftwareSerial.h>

#define NUM_CONN 3

SoftwareSerial* bus[NUM_CONN];
int RX[NUM_CONN] = {A1, A3, A5};
int TX[NUM_CONN] = {A0, A2, A4};
int coil[NUM_CONN] = {6, 5, 10};
volatile uint8_t *rxPortRegister[NUM_CONN];
uint8_t rxBitMask[NUM_CONN];

#define leds1pin 11
#define leds2pin 3

const int myID = 1;

int activeBus = -1;

unsigned long listenStartTime = 0;
#define listenTimeout 20

void setup(){
  for (int i=0; i<NUM_CONN; i++) {
    bus[i] = new SoftwareSerial(RX[i], TX[i]);
    bus[i]->begin(9600); //1200
    pinMode(coil[i], OUTPUT);

    uint8_t port = digitalPinToPort(RX[i]);
    rxPortRegister[i] = portInputRegister(port);
    rxBitMask[i] = digitalPinToBitMask(RX[i]);
  }

  pinMode(leds1pin, OUTPUT);
  pinMode(leds2pin, OUTPUT);

  digitalWrite(leds1pin, LOW);
  digitalWrite(leds2pin, LOW);

  Serial.begin(9600);
}

void loop(){
  if (activeBus==-1) {
    for (int i=0; i<NUM_CONN; i++) {
      if (!rxRead(i)) {
        setActiveBus(i);
      }
    }
  }
  else {
    readActiveBus();
  }
}

uint8_t rxRead(int index) {
  return *rxPortRegister[index] & rxBitMask[index];
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
