// resilient network
#include <SoftwareSerialWithHalfDuplex.h>
#include "Arduino.h"
#include "timers.h"

#define isSource false

#define leds1pin 11
#define leds2pin 3
#define button1pin 4

SoftwareSerialWithHalfDuplex* bus[3];

int RX[3] = {A0, A2, A4};
int TX[3] = {A1, A3, A5};
int coil[3] = {6, 5, 10};
int distanceToSource[3];
int shortestDistanceToSource;
int lampState;
int alertState;
int alertTimer;

void sendEnergy() {
  char c = 0;
  for (int i=0; i<3; i++) bus[i]->write(c);
  addTimer(100, sendEnergy);
}

void switchLampOn() {
  digitalWrite(leds1pin, HIGH);
  digitalWrite(leds2pin, HIGH);
  lampState = true;
}

void switchLampOff() {
  digitalWrite(leds1pin, LOW);
  digitalWrite(leds2pin, LOW);
  lampState = false;
}

void resetConnectors() {
  for (int i=0; i<3; i++) digitalWrite(coil[i], HIGH);
}

void dropConnector() {
  // send out message that we're going to drop a connector
  char c = 254;
  for (int i=0; i<3; i++) bus[i]->write(c);
  
  // pick random connector to drop
  int n = 0;
  for (int i=0; i<3; i++) if (distanceToSource[i]<99) n++;
  n = random(1,n+1);
  int i=0;
  while(n) {
    if(distanceToSource[i]<99) n--;
    i++;
  }
  
  // give communication a break and drop connector
  delay(10);
  digitalWrite(coil[i-1], LOW);
  addTimer(1000, resetConnectors);
  
  // relax alert state
  alertState = false;
}

void processEnergy() {
  // assess energy supply
  shortestDistanceToSource = 99;
  for (int i =0; i<3; i++) if (distanceToSource[i] < shortestDistanceToSource) shortestDistanceToSource = distanceToSource[i];
  
  // if we have energy, switch (or keep) lamp on, pass it on and check again in some time
  if (shortestDistanceToSource<99) {
    if (!lampState) switchLampOn();
    for (int i =0; i<3; i++) if (distanceToSource[i] > shortestDistanceToSource) bus[i]->write(shortestDistanceToSource);
    addTimer(100, processEnergy);
  }
  // if not, switch off the lamp and drop connectors if we were cut out
  else {
    if (lampState) {
      switchLampOff();
      for(int i=0; i<3; i++) digitalWrite(coil[i], LOW);
      addTimer(1000, resetConnectors);
    }
  }
  
  for (int i=0; i<3; i++) distanceToSource[i] = 99;
}

void setup() {
  // set up pins
  for (int i=0; i<3; i++) bus[i] = new SoftwareSerialWithHalfDuplex(RX[i], TX[i], false, false);
  for (int i=0; i<3; i++) pinMode(coil[i], OUTPUT);
  pinMode(leds1pin, OUTPUT);
  pinMode(leds2pin, OUTPUT);
  pinMode(button1pin, INPUT_PULLUP);
  
  // set initial states
  for (int i=0; i<3; i++) bus[i]->begin(1200);
  resetConnectors();
  switchLampOff();
  for (int i=0; i<3; i++) distanceToSource[i] = 99;
  shortestDistanceToSource = 99;
  alertState = false;
  
  // seed random generator
  randomSeed(analogRead(7));
  
  // start sending energy if we're a source
  if (isSource) addTimer(100, sendEnergy);
}

void loop() {
  if (isSource) {
    if (!digitalRead(button1pin)) {
      char c = 255;
      for (int i=0; i<3; i++) bus[i]->write(c);
    }
  }
  
  // check all lines for incoming data
  for (int i=0;i<3;i++) if(bus[i]->available()) {
    char c = bus[i]->read();
    switch(c) {
      case 255: // button is pressed
        if (!isSource && !alertState) {
          // set alert state and set random timer
          alertState = true;
          alertTimer = addTimer(random(1000, 5000), dropConnector);
          // propagate signal though network
          for (int j=0; j<3; j++) if (j!=i) bus[j]->write(c);
        }
        break;
      case 254: // some connector in the network is dropped
        // relax alert state and timer
        removeTimer(alertTimer);
        alertState = false;
        // propagate signal though network
        for (int j=0; j<3; j++) if (j!=i) bus[j]->write(c);
        break;
      default: // energy is received
        if (!isSource) {
          if (c<distanceToSource[i]+1) distanceToSource[i] = c+1;
          // if we are not connected and energy was received start timer to process energy (hopefully this does enhances asynchronous updating between the nodes
          if (shortestDistanceToSource == 99) addTimer(10, processEnergy);
        }
    }
  }
  
  // update timed processes
  handleTimers();
}
