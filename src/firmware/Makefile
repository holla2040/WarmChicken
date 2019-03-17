TARGET                  = arduino
MCU                     = atmega328p
#MCU                    = atmega1280
#MCU                    = atmega2560
DEVICE_FILE             = ttyUSB0
#DEVICE_FILE            = ttyACM0
#DEVICE_FILE             = rfcomm4

CONSOLE_BAUDRATE        = 57600
AVRDUDE_ARD_BAUDRATE    = 57600
#AVRDUDE_ARD_BAUDRATE   = 115200

ARDUINO_DIR             = /opt/arduino
AVR_TOOLS_PATH          = /usr/bin
AVRDUDE_ARD_PROGRAMMER  = stk500v1
AVRDUDE                 = avrdude
AVRDUDE_CONF			= ~/r/a/arduino/conf/avrdude.conf
                                            
F_CPU					= 16000000
ARDUINO_PORT            = /dev/$(DEVICE_FILE)
TCP_PORT                = net:192.168.0.41:23

include ./Arduino.mk
                                            
ARDUINO_LIBS            = Stepper/src
                                            

runold:	reset all raw_upload
	make con

runserial: all
	stty -F $(ARDUINO_PORT) $(CONSOLE_BAUDRATE)
	make reset
	#echo -en "R" > $(ARDUINO_PORT)
	#sleep 0.5
	make raw_upload
	make con

run: all
	echo -n 'R0 ' | nc 192.168.0.41 23
	make tcp_upload

con:
	rm -rf /var/lock/LCK..$(DEVICE_FILE)
	microcom -p $(ARDUINO_PORT) -s $(CONSOLE_BAUDRATE)

push: all
	/usr/bin/avrdude -V -C/home/holla/arduino/hardware/tools/avr/etc/avrdude.conf -v -patmega328p -carduino  -P net:192.168.0.10:23 -D -U flash:w:/tmp/build/arduino.hex:i

usb: all
	/usr/bin/avrdude -V -C/home/holla/arduino/hardware/tools/avr/etc/avrdude.conf -v -patmega328p -carduino  -P /dev/ttyUSB0 -b57600 -D -U flash:w:/tmp/build/arduino.hex:i

ota: all
	/home/holla/arduino/hardware/tools/avr/bin/avrdude -V -C/home/holla/arduino/hardware/tools/avr/etc/avrdude.conf -v -patmega328p -carduino  -P $(TCP_PORT) -D -U flash:w:/tmp/build/arduino.hex:i

