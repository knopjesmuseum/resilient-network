// 09 implementing MessengerBinary2
// 10 new lightning procol (in progress)
// 11 new lightning procol (further in progress), tweaking comm settings
// 12 testing lightning (always sending trough)

#include <EEPROM.h>
#include <MessengerBinary2.h>
#include "SoftwareSerial.h"

//pins
const int rxPin1 =       2;
const int txPin1 =       3;
const int rxPin2 =       4;
const int txPin2 =       5;
const int rxPin3 =       6;
const int txPin3 =       7;
const int lightningPin = 8; // lightning button
const int lampPin =      9;
const int magnet1Pin =   10;
const int magnet2Pin =   11;
const int magnet3Pin =   12;
const int ledPin =       13;
const int genPin =       A0; // determines if generator (if > 1000)

#define NUM_MAGNETS 3
int magnetPins[] = {magnet3Pin,magnet2Pin,magnet1Pin};

// messengers 
#define MES_1 0
SoftwareSerial serial1(rxPin1,txPin1);
MessengerBinary2 messenger1(MES_1,&serial1,txPin1, receivedMessage, -1);
#define MES_2 1
SoftwareSerial serial2(rxPin2,txPin2);
MessengerBinary2 messenger2(MES_2,&serial2,txPin2, receivedMessage, -1);
#define MES_3 2
SoftwareSerial serial3(rxPin3,txPin3);
MessengerBinary2 messenger3(MES_3,&serial3,txPin3, receivedMessage, -1);
const int numMessengers = 3;
MessengerBinary2* messengers[] = {&messenger1,&messenger2,&messenger3};
int messengerSwitchInterval = 2000;
long switchMessengerTime;
int messengerIndex = 0;
MessengerBinary2* messenger;

// message types
const byte TYPE_ENERGY = 1; 
const byte TYPE_LIGHTNING = 2; 


// energy
boolean isGen = false; // is generator
float energy = 0.9;
float prevEnergy = energy;
int energyDecreaseInterval = 700; //600 (olmost); //1200; //3000 (okay); 
long energyDecreaseTime = 0;
float energyDecrease = 0.2; //0.2;
int energyIncreaseInterval = 45; //25; (skips) //50 (still); //100 (most of them); //300; //3000; //1000;
long energyIncreaseTime;
int energyId = 0;
int prevEnergyId = 0;

// lamp
#define LED_STATE_ON                 0
#define LED_STATE_OFF                1
#define LED_STATE_LOW_ENERGY         2
#define LED_STATE_LIGHTNING          3
#define LED_STATE_LIGHTNING_STRIKE   4
int ledState = LED_STATE_ON;
boolean lampBlink = false;
long prevLampBlinkTime;
boolean blinkState = true;
int lampBlinkInterval = 100;
int blinkLow = 0;
int blinkHigh = 255;

// lightning / crisis
long lightningPressedTime = 0; // the time the lightning button was pressed
int maxLightningInterval = 2000;
int prevLightningValue = -1;
int numUnits = 6; // number of users + generators (used for lightning)
int lightningID = 0;
long lightningTime = 0;
long lightningStrikeTime = 0;
int lightningDuration = 1500;
int lightningStrikeDuration = 1000;
int lightningStrikeDelay = 500;//1000
int lightningStruckMagnet;
// used to temporarily store the active magnet pins or active messenger indexes:
int numActiveConnections = NUM_MAGNETS;
int activeConnnections[NUM_MAGNETS];

byte id;

// debug
int lightCounter = 0;

void setup() 
{
  id = EEPROM.read(0);
  
  Serial.begin(9600);
  Serial.print("h ");
  Serial.println(id);
  
  pinMode(genPin, INPUT);
  pinMode(lightningPin, INPUT);
  digitalWrite(lightningPin,HIGH);
  pinMode(lampPin, OUTPUT);
  pinMode(magnet1Pin, OUTPUT);
  pinMode(magnet2Pin, OUTPUT);
  pinMode(magnet3Pin, OUTPUT);
  pinMode(ledPin, OUTPUT);
  
  messenger2.debug = true;
  messenger = &messenger3; 
  messenger->listen();
}

void loop()
{
  if(millis() > energyDecreaseTime)
  {
    energy -= energyDecrease;
    if(energy < 0) energy = 0;
    energyDecreaseTime = millis()+energyDecreaseInterval;
  }
  
  //// communication
  if(millis() > switchMessengerTime && !messenger->listening)
  {
    
    messengerIndex++;
    messengerIndex %= numMessengers;
    //Serial.print("sw ");
    //Serial.println(messengerIndex);
    messenger = messengers[messengerIndex];
    messenger->listen();
    //switchMessengerTime = millis()+random(40,200); // 6 sec energy not enough (for 2nd conn)
    //switchMessengerTime = millis()+random(80,240); // can take 4 to 5 sec
    //switchMessengerTime = millis()+random(80,160); // slightly less frequent 4 to 6 seconds
    //switchMessengerTime = millis()+random(40,160); // even 3 seconds is rare, so better
    //switchMessengerTime = millis()+random(30,150); // slightly better, mostly 1 - 2 seconds
    //switchMessengerTime = millis()+random(30,200); // slightly worse (more times it takes longer)
    //switchMessengerTime = millis()+random(30,60); // better, mostly 1 second, up to 3 seconds (not usable when debug)
    //switchMessengerTime = millis()+random(30,40); // even faster, but can get stuck for 5 seconds
    //switchMessengerTime = millis()+random(5,20); // interesting...
    //switchMessengerTime = millis()+random(5,10); // slighty slower?
    //switchMessengerTime = millis()+random(2,10); // still works (missing lightning message?)
    //switchMessengerTime = millis()+random(5,10); // still missing messages
    
    switchMessengerTime = millis()+random(20,40); 
    switchMessengerTime = millis()+random(20,80); 
  }
  for(int i=0;i<numMessengers;i++)
    messengers[i]->update();
  //messenger3.update();
  //messenger->update();
  
  //// generator
  isGen = (analogRead(genPin) > 1000);
  if(isGen && millis() > energyIncreaseTime)
  {
    energy = 1;
    // send energy around with id that counts up
    for(int i=0;i<numMessengers;i++)
      sendMessage(i,TYPE_ENERGY,energyId,true,false); //messengers[i]->sendMessage('E',energyId,true);
      
    energyId++;
    energyId %= 8; //limited by 3 bits max value
    energyIncreaseTime = millis()+energyIncreaseInterval;
  }
  
  //// lightning
  if(digitalRead(lightningPin) == LOW && millis() > lightningPressedTime+maxLightningInterval)
  {
    Serial.print("l ");
    Serial.println(lightningID);
    
    lightningPressedTime = millis();
    
    //TODO: lower change of striking
    //boolean lightningStruck = lightning(lightningID,numUnits*2); //(lower change to prevent always lightning at source)
    boolean lightningStruck = false; //lightning(0,4); //(lower change to prevent always lightning at source)
    if(!lightningStruck) // no strike? send through
    {
      // determine candidate connections to send the lightning to
      numActiveConnections = 0;
      for(int i=0;i<NUM_MAGNETS;i++)
      {
        if(messengers[i]->isAlive())
        {
          activeConnnections[numActiveConnections] = messengers[i]->index;
          numActiveConnections++;
        }
      }
      if(numActiveConnections > 0)
      {
        int randomConnection = round(random(0,numActiveConnections-1));
        int lightningStruckConnection = activeConnnections[randomConnection];
        Serial.print("  nls ");
        Serial.println(randomConnection);
        sendMessage(lightningStruckConnection,TYPE_LIGHTNING,lightningID,false,false);
      }
    }
    lightningID++;
    lightningID %= 8; //limited by 3 bits
  }
  //Serial.print(prevEnergy);
  //Serial.print(' ');
  //Serial.println(energy);
  // check energy consequences
  if(energy != prevEnergy) updateActuators();
  
  //Serial.print(' ');
  //Serial.println(ledState);
  //// lamp
  switch(ledState)
  {
    case LED_STATE_ON:
      digitalWrite(lampPin,HIGH);
      lampBlink = false;
      break;
    case LED_STATE_OFF:
      digitalWrite(lampPin,LOW);
      lampBlink = false;
      break;
    case LED_STATE_LOW_ENERGY:
      lampBlinkInterval = 100; //30
      blinkLow = 0;
      blinkHigh = 100;
      lampBlink = true;
      break;
    case LED_STATE_LIGHTNING:
      lampBlinkInterval = 50; //30
      blinkHigh = 255;
      //blinkLow = 150;
      blinkLow = 200;
      lampBlink = true;
      break;
    case LED_STATE_LIGHTNING_STRIKE:
      lampBlinkInterval = 50; //30
      blinkHigh = 255;
      blinkLow = 0;
      lampBlink = true;
      break;
  }
  if(lampBlink)
  {
    if(millis() > prevLampBlinkTime)
    {
      blinkState = !blinkState;
      analogWrite(lampPin,(blinkState)?blinkHigh:blinkLow);
      prevLampBlinkTime = millis()+lampBlinkInterval;
    }
  }
  //lightCounter--;
  //if(lightCounter < 0) lightCounter = 0; 
  //digitalWrite(ledPin, ((lightCounter > 0)? LOW : HIGH));  
  
  while (Serial.available()) {
    id = Serial.read()-48; 
    EEPROM.write(0, id);
    Serial.print("id: ");
    Serial.println(id);
  }
}
// check energy consequences etc
void updateActuators()
{
  // after lightning strike delay within and within duration?
  if(millis() > lightningStrikeTime + lightningStrikeDelay && millis() < lightningStrikeTime + lightningStrikeDelay + lightningStrikeDuration && lightningStrikeTime != 0)
  {
    //digitalWrite(lightningStruckMagnet,LOW);
    ledState = LED_STATE_LIGHTNING_STRIKE;
  }
  else
  {
    // within lightning duration?
    if(millis() < lightningTime+lightningDuration && lightningTime != 0)
    {
      ledState = LED_STATE_LIGHTNING;
    }
    else
    {
      if(energy <= 0)
        ledState = LED_STATE_OFF;
      else if(energy <= 0.2)
        ledState = LED_STATE_LOW_ENERGY;
      else
        ledState = LED_STATE_ON;
      
      if(energy == 0 || energy == 1)
      {
        if(energy == 0)
        {
          Serial.println("oe");
        }
        for(int i=0;i<NUM_MAGNETS;i++) 
          digitalWrite(magnetPins[i],(energy > 0)? HIGH : LOW);
      }
    }
  }
  
  prevEnergy = energy; 
}
void receivedMessage(byte type, byte value, int index)
{
  Serial.print(index);
  Serial.print(' ');
  Serial.print(type, DEC);
  Serial.print(' ');
  Serial.println(value, DEC);
  
  //lastMessageTime[index] = millis();
    
  switch(type)
  {
    case TYPE_ENERGY: // Energy 
      //if(value != prevEnergyId)
      //{
        energy = 1;
        updateActuators();
        sendTrough(type,value,index,true,false);
        //prevEnergyId = value;
      //}
      break;
    case TYPE_LIGHTNING: // Lightning
      /*Serial.print(index);
      Serial.print(' ');
      Serial.print(type, DEC);
      Serial.print(' ');
      Serial.print(value, DEC);
      Serial.print(' ');
      Serial.println(prevLightningValue);*/
      //Serial.write('\n');
      if(value != prevLightningValue)//TODO need this?
      {
        //Serial.print(" n");
        
        //boolean lightningStruck = lightning(value,numUnits/2);
        boolean lightningStruck = false; //lightning(value,2);
        Serial.print("  L");
        Serial.println(lightningStruck);
        if(!lightningStruck) // no strike? send through
        {
          Serial.print("    st ");
          // send to random, active connection, exept the connection it came from
          numActiveConnections = 0;
          for(int i=0;i<NUM_MAGNETS;i++)
          {
            if(messengers[i]->isAlive() && i != index)
            {
              activeConnnections[numActiveConnections] = messengers[i]->index;
              numActiveConnections++;
            }
          }
          Serial.println(numActiveConnections);
          if(numActiveConnections > 0) 
          {
            int randomConnection = round(random(0,numActiveConnections-1));
            int lightningStruckConnection = activeConnnections[numActiveConnections];
            Serial.print("      sa ");
            Serial.println(randomConnection);
            sendMessage(lightningStruckConnection,TYPE_LIGHTNING,lightningID,false,false);
          }
          /*else // send back to origin
          {
            Serial.println("      so ");
            sendMessage(index,TYPE_LIGHTNING,lightningID,false,false);
          }*/
        }
        prevLightningValue = value;
      }
      else
      {
        Serial.write('\n');
      }
      break;
  }
}
void sendTrough(byte type, byte value, int origin, boolean overrideSameType,boolean aliveCheck)
{
  /*Serial.print('s');
  Serial.print('t');
  Serial.print(origin);
  Serial.print(' ');*/
  // update neighbors, except origin
  for(int i=0;i<numMessengers;i++)
  {
    if(messengers[i]->index != origin)
    {
      //Serial.print(messengers[i]->index);
      sendMessage(i,type,value,overrideSameType,aliveCheck);//messengers[i]->sendMessage(type,value,overrideSameType);
    }
  }
  //Serial.write('\n');
}
void sendMessage(int target,int type, int value, boolean overrideSameType, boolean aliveCheck)
{
  if(!aliveCheck || (aliveCheck && messengers[target]->isAlive()))//millis()-lastMessageTime[target] < maxMessageDelay))
    messengers[target]->sendMessage(type,value,overrideSameType);
}
boolean lightning(int lightningID, int randomMax)
{
  Serial.println("fL");
  lightningTime = millis();
  
  int respondValue = round(random(randomMax));
  boolean lightningStrike = (respondValue == lightningID); // Do something with lightning?
  if(lightningStrike)
  {
    Serial.print("  ls ");
    Serial.println(respondValue);
    lightningStrikeTime = millis();
    
    // determine candidate magnets for strike with lightning (release)
    numActiveConnections = 0;
    for(int i=0;i<NUM_MAGNETS;i++)
    {
      if(messengers[i]->isAlive())
      {
        activeConnnections[numActiveConnections] = magnetPins[i];
        numActiveConnections++;
      }
    }
    
    if(numActiveConnections > 0)
    {
      Serial.print("    ");
      Serial.print(numActiveConnections); 
      int randomMagnet = round(random(0,numActiveConnections-1));
      Serial.print(' ');
      lightningStruckMagnet = activeConnnections[randomMagnet];
      Serial.println(lightningStruckMagnet);
      //digitalWrite(lightningStruckMagnet,LOW);
    }
  }
  return lightningStrike;
}
