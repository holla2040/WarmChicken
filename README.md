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
|procedural programming         |firmware/app.pde           |C compiler     |
|object oriented programming    |firmware/WarmDirt class    |C++ compiler   |
|digital design                 |WarmDirt circuit board     |ATmega328P        |
|sensing                        |ultrasonic, PTC Rs, etc    |various        |
|mechanical                     |door pulley                |FreeCad        |
|additive fabrication           |door pulley                |FlashForge 3D printer |
|motor control                  |WarmDirt motor driver      |L6207D         |
|lighting control               |WarmDirt motor driver      |L6207D         |
|wireless communication         |WarmDirt WiFi radio        |WiFly RN-XV    |
|carbon footprint               |chicken coop               |Solar powered, 10W panel  |
|device firmware update         |WarmDirt processor+WiFi    |Arduino bootloader|
|data exchange                  |firmware/app.pde           |JSON key/value pairs|
|time series data store         |RPI2 server                |mongodb        |
|caching data store             |RPI2 server                |redis          |
|web framework                  |RPI2 server                |meteorJS on node      |
|responsive layout              |browser webkit             |bootstrap CSS  |
|single page application        |browser webkit             |meteor blaze   |
|real-time updates              |browser webkit & server    |meteor DDP over websockets |
|hardware dashboard             |RPI2 console               |X surf fullscreen on Adafruit TFT|





