Resilient network PCB
=====================
Contains, with Atmega on the right: 
- 2 led groups (pins: 11, 3)
- 3x bus system (bottom side) (pins: A0 + A1 / A2 + A3 / A4 + A5)
- 3x magnet controlling mosfets (left side) (pins: 6, 5, 10)
- Power regulation
- Footprint for Atmega 328p tqfp, [pins](https://raw.githubusercontent.com/knopjesmuseum/resilient-network/master/pcb/atmega328-tqfp-arduino-pinout.jpg)
- 2 programming connections: ISP and FTDI.
- 2 button footprints, one used for disturbance button (top side) (pins: 2, 4)
- 6 debug leds, from left to right: 
  1. Magnet / coil 1 enabled
  2. Magnet / coil 2 enabled
  3. Magnet / coil 3 enabled
  4. Software controllable debug led (or + / S pads enabled) (pin 9) 
  5. Led 1 group enabled
  6. Led 2 group enabled
- Currently unused extra pins, for future fun:
  - The middle two pins of the group of 4 pins on the top (pins: 7, 8). 
    Diodes or leds can be placed in between for protection. 
  - The bigger + / S pads top left. Probably connected to pin 9 through mosfet. 
  - Big pad top right of hole. For PIR sensor? (pin: A6)
  - Big pad bottom left of hole. For PIR sensor? (pin: A7)

Bus system
----------
To get RX and TX over one cable we need "half duplex" serial communication. We're using the following circuit to enable this. This makes sure the signals send on the TX aren't picked up by the RX. 
![Half duplex circuit](https://raw.githubusercontent.com/knopjesmuseum/resilient-network/master/pcb/half-duplex-circuit.png)
The idea is better described in the following post: http://nerdralph.blogspot.ca/2014/01/avr-half-duplex-software-uart.html 
We don't use that software library though, we use the regular [SoftwareSerial](https://www.arduino.cc/en/Reference/SoftwareSerial). 
- Port A: RX: A1, TX: A0;
- Port B: RX: A3, TX: A2;
- Port C: RX: A5, TX: A4;

FTDI
--------------------
Note; auto reset will only work when there is a 0,1 uF capacitor between the FTDI RST pin and the atmega reset pin.
You can also connect a TTY connector by connecting to just the FTDI's gnd, rx and tx pins. 

Bill of materials
--------------------
- Atmega 328 tqfp (Arduino Uno)
- [3x Intertec electromagnet 12 Vdc M3](  http://www.conrad.com/ce/en/product/502290/Intertec-ITS-MS-2015-12VDC-electromagnet-12-Vdc-M3)
