#include <SoftwareSerial.h>

#define NUM_CONN 3

SoftwareSerial* bus[NUM_CONN];
int RX[NUM_CONN] = {A1, A3, A5};
int TX[NUM_CONN] = {A0, A2, A4};

#define leds1pin 11
#define leds2pin 3

#define MAX_COUNTER 1000; //4294967295;
unsigned long counter = MAX_COUNTER;
boolean ledEnabled = false;

int targetBusIndex = 0;

void setup(){
  pinMode(leds1pin, OUTPUT);
  pinMode(leds2pin, OUTPUT);
  digitalWrite(leds1pin, LOW);
  digitalWrite(leds2pin, LOW);

  for (int i=0; i<NUM_CONN; i++) {
    bus[i] = new SoftwareSerial(RX[i], TX[i]);
    bus[i]->begin(9600); //1200
  }

  Serial.begin(9600);
}

void loop(){
  // Forward incomming serial communication to all software serial ports
  if( Serial.available()){
    counter = MAX_COUNTER; // light incomming serial communicating led
    int index = int(Serial.read())-48;
    // int index = Serial.read();
    for (int i=0;i<NUM_CONN;i++) {
      // begin with pulse so other side can start listening
      digitalWrite(TX[i], LOW);
      delay(2);
      digitalWrite(TX[i], HIGH);
      bus[i]->write(index);
    }
  }
  // update incomming serial communicating led
  updateLed((counter > 0));
  if(counter > 0) counter --;
}

void updateLed(boolean enable) {
  if(enable != ledEnabled) {
    digitalWrite(leds1pin, enable? HIGH : LOW);
    ledEnabled = enable;
  }
}
