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

int activeBus = -1;

unsigned long listenStartTime = 0;
#define listenTimeout 20

#define SERIAL_DEBUG false

// #define MAX_WRITES 5
// int writeCounter = 0;

void setup(){
  for (int i=0; i<NUM_CONN; i++) {
    bus[i] = new SoftwareSerial(RX[i], TX[i]);
    bus[i]->begin(115200); //(9600); //1200
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
  if(Serial.available()){
    int index = int(Serial.read())-48;
    // writeCounter = 0;
    txWrite(0, index);
  }
}

uint8_t rxRead(int index) {
  return *rxPortRegister[index] & rxBitMask[index];
}

void setActiveBus(int index) {
  if(SERIAL_DEBUG) {
    Serial.print("l");
    Serial.println(index);
  }
  activeBus = index;
  listenStartTime = millis();
  bus[index]->listen();
}

void readActiveBus() {
  while(bus[activeBus]->available()) {
    if(SERIAL_DEBUG) {
      Serial.print("a");
      Serial.println(activeBus);
    }
    int index = int(bus[activeBus]->read());
    if(index != 0) {
      // Serial.println(index);
      toggleCoil(activeBus);
      int targetBus = activeBus+1;
      if(targetBus > NUM_CONN-1) targetBus=0;
      txWrite(targetBus,index);
      // since we're only sending one thing, after reading it we start checking other ports again
      activeBus = -1;
      break;
    }
  }
  if (millis()-listenStartTime > listenTimeout) {
    if(SERIAL_DEBUG) {
      Serial.print("t");
      Serial.println(activeBus);
    }
    activeBus = -1;
  }
}

void txWrite(int index, int value) {
  // writeCounter++;
  // if(writeCounter < MAX_WRITES) return;
  if(SERIAL_DEBUG) {
    Serial.print("w");
    Serial.println(index);
  }
  // begin with pulse so other side can start listening
  digitalWrite(TX[index], LOW);
  delay(2);
  digitalWrite(TX[index], HIGH);
  bus[index]->write(value);
}

void toggleCoil(int index) {
  digitalWrite(coil[index], !digitalRead(coil[index]));
}
