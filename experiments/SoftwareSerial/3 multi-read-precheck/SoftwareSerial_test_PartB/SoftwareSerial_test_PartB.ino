#include <SoftwareSerial.h>

#define NUM_CONN 3 

SoftwareSerial* bus[NUM_CONN];
int RX[NUM_CONN] = {A1, A3, A5};
int TX[NUM_CONN] = {A0, A2, A4};

int coils[NUM_CONN] = {6, 5, 10};
boolean coilEnabled[NUM_CONN] = {false, false, false};

#define leds1pin 11
#define leds2pin 3

#define MAX_COUNTER 1000; //4294967295;
unsigned long counter = MAX_COUNTER;
boolean ledEnabled = false; 

volatile uint8_t *_rx0PortRegister;
uint8_t _rx0BitMask;
volatile uint8_t *_rx1PortRegister;
uint8_t _rx1BitMask;
volatile uint8_t *_rx2PortRegister;
uint8_t _rx2BitMask;

int targetBusIndex = 0;

void setup(){
  pinMode(leds1pin, OUTPUT);
  pinMode(leds2pin, OUTPUT);
  digitalWrite(leds1pin, LOW);
  digitalWrite(leds2pin, LOW);
  
  for (int i=0; i<NUM_CONN; i++) {
    bus[i] = new SoftwareSerial(RX[i], TX[i]);
    bus[i]->begin(9600); //1200
    pinMode(coils[i], OUTPUT);
  }
  
  Serial.begin(9600);
  
  uint8_t port0 = digitalPinToPort(RX[0]);
  _rx0PortRegister = portInputRegister(port0);
  _rx0BitMask = digitalPinToBitMask(RX[0]);
  
  uint8_t port1 = digitalPinToPort(RX[1]);
  _rx1PortRegister = portInputRegister(port1);
  _rx1BitMask = digitalPinToBitMask(RX[1]);
  
  uint8_t port2 = digitalPinToPort(RX[2]);
  _rx2PortRegister = portInputRegister(port2);
  _rx2BitMask = digitalPinToBitMask(RX[2]);
}

void loop(){
  
  if (!rx0Read()) {
    updateCoil(0, true);
    readBus(0);
  }
  if (!rx1Read()) {
    updateCoil(1, true);
    readBus(1);
  }
  if (!rx2Read()) {
    updateCoil(2, true);
    readBus(2);
  }
  
  if( Serial.available()){
    counter = MAX_COUNTER;
    int index = int(Serial.read())-48;
    for (int i=0;i<NUM_CONN;i++) {
      // bus[i]->write(index);
      digitalWrite(TX[i], LOW);
      delay(1);
      digitalWrite(TX[i], HIGH);
      bus[i]->write(index);  
    }
    // digitalWrite(TX[targetBusIndex], LOW);
    // delay(1);
    // digitalWrite(TX[targetBusIndex], HIGH);
    // bus[targetBusIndex]->write(index);
    
    // targetBusIndex++;
    // if(targetBusIndex > 2) targetBusIndex = 0;
  }
  updateLed((counter > 0));
  if(counter > 0) counter --;
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

void readBus(uint8_t index) {
  bus[index]->listen();
  if( bus[index]->available() ){
    char c = bus[index]->read();
    Serial.write(c);
  }
}

void updateCoil(uint8_t pinIndex, boolean enable) {
  if(enable != coilEnabled[pinIndex]) {
    digitalWrite(coils[pinIndex], enable? HIGH : LOW);
    coilEnabled[pinIndex] = enable;
  }
}

void updateLed(boolean enable) {
  if(enable != ledEnabled) {
    digitalWrite(leds1pin, enable? HIGH : LOW);
    ledEnabled = enable;
  }
}
