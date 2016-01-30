// resilient network
#include <SoftwareSerial.h>
#include "Arduino.h"
#include "timers.h"

#define leds1pin 11
#define leds2pin 3
#define button1pin 2
#define button2pin 4

#define SERIAL_DEBUG true

#define DROPPING_CONNECTOR 254
#define ALERT 255

#define NUM_CONN 3

SoftwareSerial* bus[NUM_CONN];

int RX[NUM_CONN] = {A1, A3, A5};
int TX[NUM_CONN] = {A0, A2, A4};
int coil[NUM_CONN] = {6, 5, 10}; // magnets
int distanceToSource[NUM_CONN] = {99, 99, 99};
int shortestDistanceToSource;
int lampState;
int alertState;
int alertTimer;
boolean isSource = false;

void sendEnergy() {
  if(SERIAL_DEBUG) Serial.println("send energy");
  // source sends energy, as 0, meaning 0 distance to source
  char distance = 0;
  for (int i=0; i<NUM_CONN; i++) bus[i]->write(distance);
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
  for (int i=0; i<NUM_CONN; i++) digitalWrite(coil[i], HIGH);
}

void dropConnector() {
  // send out message that we're going to drop a connector
  char c = DROPPING_CONNECTOR;
  for (int i=0; i<NUM_CONN; i++) bus[i]->write(c);

  // pick random connected connector to drop
  // count number of ports with a low distance to source
  int n = 0;
  for (int i=0; i<NUM_CONN; i++) if (distanceToSource[i]<99) n++;
  n = random(1,n+1);
  int i=0;
  // go through ports, only count connecteds ports, untill random number is empty
  while(n) {
    if(distanceToSource[i]<99) n--;
    i++;
  }

  if(SERIAL_DEBUG) {
    Serial.println("dropping connector");
  }

  // give communication a break and drop connector
  delay(10);
  digitalWrite(coil[i-1], LOW);
  addTimer(1000, resetConnectors);

  // relax alert state
  alertState = false;
}

// ToDo: why not call this on a fixed interval?
void processEnergy() {
  // assess energy supply, find shortest disance to source
  shortestDistanceToSource = 99;
  for (int i =0; i<NUM_CONN; i++) if (distanceToSource[i] < shortestDistanceToSource) shortestDistanceToSource = distanceToSource[i];

  if(SERIAL_DEBUG) {
    Serial.print("sd: ");
    Serial.println(shortestDistanceToSource);
  }

  // if we have energy, switch (or keep) lamp on, pass it on and check again in some time
  if (shortestDistanceToSource<99) {
    if (!lampState) switchLampOn();
    // send shortest distance to ports with a higher distance
    for (int i =0; i<NUM_CONN; i++) if (distanceToSource[i] > shortestDistanceToSource) bus[i]->write(shortestDistanceToSource);
    addTimer(100, processEnergy);
  }
  // if not, switch off the lamp and drop connectors if we were cut out
  else {
    if (lampState) {
      switchLampOff();
      for(int i=0; i<NUM_CONN; i++) digitalWrite(coil[i], LOW);
      addTimer(1000, resetConnectors);
    }
  }

  for (int i=0; i<NUM_CONN; i++) distanceToSource[i] = 99;
}

void setup() {
  // set up pins
  for (int i=0; i<NUM_CONN; i++) {
    bus[i] = new SoftwareSerial(RX[i], TX[i]);
    pinMode(coil[i], OUTPUT);
  }
  pinMode(leds1pin, OUTPUT);
  pinMode(leds2pin, OUTPUT);
  pinMode(button1pin, INPUT_PULLUP);
  pinMode(button2pin, INPUT_PULLUP);

  // set initial states
  for (int i=0; i<NUM_CONN; i++) bus[i]->begin(1200);
  resetConnectors();

  for (int i=0; i<NUM_CONN; i++) distanceToSource[i] = 99;
  shortestDistanceToSource = 99;
  alertState = false;

  // seed random generator
  randomSeed(analogRead(7));

  // start sending energy if we're a source
  isSource = digitalRead(button1pin) == LOW;
  if (isSource) {
    addTimer(100, sendEnergy);
    switchLampOn(); // for debugging
  } else {
    switchLampOff();
    processEnergy();
  }

  if(SERIAL_DEBUG) {
    Serial.begin(9600);
    Serial.println("setup");
    Serial.print("isSource: ");
    Serial.println(isSource);
  }
}

void loop() {
  if (isSource) {
    if (!digitalRead(button2pin)) {
      if(SERIAL_DEBUG) Serial.println("alert");
      char c = ALERT;
      for (int i=0; i<NUM_CONN; i++) bus[i]->write(c);
    }
  }

  // check all lines for incoming data
  for (int i=0;i<NUM_CONN;i++) if(bus[i]->available()) {
    char c = bus[i]->read();
    if(SERIAL_DEBUG) {
      Serial.print(i);
      Serial.print(": ");
      Serial.println(int(c));
    }
    switch(c) {
      case ALERT: // button is pressed
        if (!isSource && !alertState) {
          // set alert state and set random timer
          alertState = true;
          alertTimer = addTimer(random(1000, 5000), dropConnector);
          // propagate signal though network
          for (int j=0; j<NUM_CONN; j++) if (j!=i) bus[j]->write(c);
        }
        break;
      case DROPPING_CONNECTOR: // some connector in the network is dropped
        if(alertState) {
          // relax alert state and timer
          removeTimer(alertTimer); // cancel dropping connector
          alertState = false;
          // propagate signal though network
          for (int j=0; j<NUM_CONN; j++) if (j!=i) bus[j]->write(c);
        }
        break;
      default: // distance to source
        if (!isSource) {
          // if new distance is shorter, take that as distance to source for this port
          if (c<distanceToSource[i]+1) distanceToSource[i] = c+1;
          // if we are not connected (yet) and a new distance was received start timer to process energy
          // (hopefully this does enhances asynchronous updating between the nodes)
          // ToDo: why use timer here? can't it get triggered to much, shouldn't this simply debounce? why not call on fixed interval?
          if (shortestDistanceToSource == 99) addTimer(10, processEnergy);
        }
    }
  }

  // update timed processes
  handleTimers();
}
