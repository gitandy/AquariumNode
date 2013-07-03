AquariumNode
============

AquariumNode is an Arduino sketch for a JeeNode to control aquarium temperature.

The AquariumNode controls a 12V fan with PWM to cool down the water temperature on very hot days. 
It sends back the current set point, water temperature and fan speed. 

You can send the temperature set point and the PI regulator parameters (proportinal and integral gain) to the node. 
The parameters are stored in eeprom for the case of power loss.
