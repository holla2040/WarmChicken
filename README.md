WarmChicken
===========

Chicken Coop Controller

Controller uses WarmDirt hardware

Operation:
a. Control door position based on sunlight level
c. Control interior temperature
d. Control interior lighting to provide more than 8 hours of light/day



|Technology         |Location                   |Implementation |
|-------------------|---------------------------|---------------|
|procedural         |firmware/app.pde           |C compiler     |
|oop                |firmware/WarmDirt class    |C++ compiler   |
|digital design     |WarmDirt circuit board     |avr328P        |
|sensing            |ultrasonic, PTC Rs, etc    |various        |
|mechanical         |door pulley                |FreeCad        |
|3D fabrication     |door pulley                |FlashForge 3D printer |
|motor control      |WarmDirt motor driver      |L6207D         |
|wireless comm      |WarmDirt WiFi radio        |WiFly RN-XV    |
|DFU                |WarmDirt processor+WiFi    |Arduino bootloader|


