// resilient network
#include <SoftwareSerial.h>
#include "Arduino.h"
#include "timers.h"

#define leds1pin 11
#define leds2pin 3
#define button1pin 2 // isSource jumper
#define button2pin 4 // alert button

#define SERIAL_DEBUG true

#define DROPPING_CONNECTOR 254
#define ALERT 255

#define NUM_CONN 3

SoftwareSerial* bus[NUM_CONN];
int RX[NUM_CONN] = {A1, A3, A5};
int TX[NUM_CONN] = {A0, A2, A4};
volatile uint8_t *rxPortRegister[NUM_CONN];
uint8_t rxBitMask[NUM_CONN];
int activeBus = -1;
unsigned long listenStartTime = 0;
#define listenTimeout 20

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
  // Note: using distance 1, because initial pulse is read as 0
  char distance = 1;
  for (int i=0; i<NUM_CONN; i++) txWrite(i, distance);
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
  for (int i=0; i<NUM_CONN; i++) txWrite(i, DROPPING_CONNECTOR);

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
    for (int i =0; i<NUM_CONN; i++) if (distanceToSource[i] > shortestDistanceToSource) txWrite(i, shortestDistanceToSource);
    addTimer(500, processEnergy);
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
  //addTimer(50, processEnergy);
}

uint8_t rxRead(int busIndex) {
  return *rxPortRegister[busIndex] & rxBitMask[busIndex];
}

void txWrite(int index, int value) {
  // writeCounter++;
  // if(writeCounter < MAX_WRITES) return;
  if(SERIAL_DEBUG) {
    Serial.print("w");
    Serial.print(busIndex);
  }
  // begin with pulse so other side can start listening
  digitalWrite(TX[busIndex], LOW);
  delay(2);
  digitalWrite(TX[busIndex], HIGH);
  bus[busIndex]->write(value);
}

void setActiveBus(int busIndex) {
  if(SERIAL_DEBUG) {
    Serial.print("l");
    Serial.println(busIndex);
  }
  activeBus = busIndex;
  listenStartTime = millis();
  bus[activeBus]->listen();
}

void readActiveBus() {
  while(bus[activeBus]->available()) {
    if(SERIAL_DEBUG) {
      Serial.print("a");
      Serial.println(activeBus);
    }
    int message = int(bus[activeBus]->read());
    if(message != 0) {
      if(SERIAL_DEBUG) {
        Serial.print(activeBus);
        Serial.print(": ");
        Serial.println(message);
      }
      parseMessage(activeBus, message);
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

void parseMessage(int busIndex, int message) {
  switch(message) {
    case ALERT: // button is pressed
      if (!isSource && !alertState) {
        // set alert state and set random timer
        alertState = true;
        alertTimer = addTimer(random(1000, 5000), dropConnector);
        // propagate signal though network
        for (int j=0; j<NUM_CONN; j++) if (j!=busIndex) txWrite(j, ALERT);
      }
      break;
    case DROPPING_CONNECTOR: // some connector in the network is dropped
      if(alertState) {
        // relax alert state and timer
        removeTimer(alertTimer); // cancel dropping connector
        alertState = false;
        // propagate signal though network
        for (int j=0; j<NUM_CONN; j++) if (j!=busIndex) txWrite(j, DROPPING_CONNECTOR);
      }
      break;
    default: // distance to source
      if (!isSource) {
        // if new distance is shorter, take that as distance to source for this port
        if (message<distanceToSource[busIndex]+1) distanceToSource[busIndex] = message+1;
        // if we are not connected (yet) and a new distance was received start timer to process energy
        // (hopefully this does enhances asynchronous updating between the nodes)
        // ToDo: why use timer here? can't it get triggered to much, shouldn't this simply debounce? why not call on fixed interval?
        if (shortestDistanceToSource == 99) addTimer(10, processEnergy);
      }
  }
}

void setup() {
  // set up pins
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

  isSource = digitalRead(button1pin) == LOW;
  if (isSource) {
    // start sending energy
    addTimer(100, sendEnergy);
    switchLampOn(); // for debugging
  } else {
    // start processing energy
    processEnergy();
    switchLampOff();
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
    if (!digitalRead(button2pin)) { // ToDo: use bitmask
      if(SERIAL_DEBUG) Serial.println("alert");
      char c = ALERT;
      for (int i=0; i<NUM_CONN; i++) txWrite(i, ALERT);
    }
  }

  // check all lines for incoming data
  if (activeBus==-1) {
    for (int i=0; i<NUM_CONN; i++) {
      if (!rxRead(i)) {
        setActiveBus(i);
      }
    }
  } else {
    readActiveBus();
  }

  // update timed processes
  handleTimers();
}
