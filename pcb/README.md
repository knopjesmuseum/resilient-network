Resilient network PCB
=====================
Contains, with atmega on the right: 
- 2 led groups (pins: 11, 3)
- 3x bus system (bottom side) (pins: A0 + A1 / A2 + A3 / A4 + A5)
- 3x magnet controlling mosfets (left side) (pins: 6, 5, 10)
- power regulation
- Footprint for Atmega 328p 
- 2 programming connections: ISP and serial
- 2 button footprints, one used for disturbance button (top side) 
- 6 debug leds, from left to right: 
  1. Magnet / coil 1 enabled
  2. Magnet / coil 2 enabled
  3. Magnet / coil 3 enabled
  4. ...
  5. Led 1 group enabled
  6. Led 2 group enabled

Bus system
----------
Follows idea: http://nerdralph.blogspot.ca/2014/01/avr-half-duplex-software-uart.html  

Bill of materials
--------------------

- Atmega 328 (Arduino Uno)
- 3x Intertec electromagnet 12 Vdc M3
  http://www.conrad.com/ce/en/product/502290/Intertec-ITS-MS-2015-12VDC-electromagnet-12-Vdc-M3
