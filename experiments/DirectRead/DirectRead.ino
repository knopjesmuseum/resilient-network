#define coil1 6
#define coil2 5
#define leds1pin 11
#define leds2pin 3
#define button1pin 2 // not used
#define btn2pin 4

#define MAX_COUNTER 2000; //4294967295;
unsigned long counter = MAX_COUNTER;
boolean coilEnabled = false;

volatile uint8_t *_btn2PortRegister;
uint8_t _btn2BitMask;

void setup(){
  
  pinMode(coil1, OUTPUT);
  pinMode(coil2, OUTPUT);
  pinMode(leds1pin, OUTPUT);
  pinMode(leds2pin, OUTPUT);
  pinMode(btn2pin, INPUT_PULLUP);
  
  digitalWrite(leds1pin, LOW);
  digitalWrite(leds2pin, LOW);
  
  uint8_t btn2Port = digitalPinToPort(btn2pin);
  _btn2PortRegister = portInputRegister(btn2Port);
  _btn2BitMask = digitalPinToBitMask(btn2pin);
}

void loop(){
  updateCoil(btn2Read());
}

uint8_t btn2Read() {
  return *_btn2PortRegister & _btn2BitMask;
}

void updateCoil(boolean enable) {
  if(enable != coilEnabled) {
    digitalWrite(coil1, enable? HIGH : LOW);
    coilEnabled = enable;
  }
}
