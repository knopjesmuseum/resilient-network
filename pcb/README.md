Resilient network PCB
=====================
Contains:
- 2 led groups
- 3x bus system, see: http://nerdralph.blogspot.ca/2014/01/avr-half-duplex-software-uart.html
- 3x magnet controlling mosfets + debug lights
- power regulation
- Footprint for Atmega 328p 
- 2 programming connections: ISP and serial
- Solder pads for disturbance button (S +)
- 6 debug leds, with atmega on the right, from left to right: 
  1. Magnet / coil 1 enabled
  2. Magnet / coil 2 enabled
  3. Magnet / coil 3 enabled
  4. ...
  5. Led 1 group enabled
  6. Led 2 group enabled

Bill of materials
--------------------

- Atmega 328 (Arduino Uno)
- 3x Intertec electromagnet 12 Vdc M3
  http://www.conrad.com/ce/en/product/502290/Intertec-ITS-MS-2015-12VDC-electromagnet-12-Vdc-M3
