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
|procedural programming        |firmware/app.pde           |C compiler     |
|object oriented programming                |firmware/WarmDirt class    |C++ compiler   |
|digital design     |WarmDirt circuit board     |ATmega328P        |
|sensing            |ultrasonic, PTC Rs, etc    |various        |
|mechanical         |door pulley                |FreeCad        |
|3D fabrication     |door pulley                |FlashForge 3D printer |
|motor control      |WarmDirt motor driver      |L6207D         |
|wireless comm      |WarmDirt WiFi radio        |WiFly RN-XV    |
|carbon footprint   |chicken coop               |Solar powered, 10W panel  |
|DFU                |WarmDirt processor+WiFi    |Arduino bootloader|
|data exchange      |firmware/app.pde           |JSON key/value pairs|
|time series data store         |RPI2 server    |mongodb        |
|caching data store         |RPI2 server        |redis          |
|web framework      |RPI2 server                |meteorJS on node      |
|responsive layout  |browser webkit             |bootstrap CSS  |
|single page design |browser webkit             |meteor blaze   |
|real-time updates  |browser webkit/server      |meteor DDP over websockets |





