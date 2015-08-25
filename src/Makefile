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
AVRDUDE_CONF 			= ~/r/a/arduino/conf/avrdude.conf
                                            
F_CPU                   = 16000000
ARDUINO_PORT            = /dev/$(DEVICE_FILE)
TCP_PORT                = net:192.168.0.35:2000
                                            
ARDUINO_LIBS            = Stepper
                                            
include ./Arduino.mk

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
	echo -n 'R0 ' | nc 192.168.0.35 2000
	make tcp_upload

con:
	rm -rf /var/lock/LCK..$(DEVICE_FILE)
	microcom -p $(ARDUINO_PORT) -s $(CONSOLE_BAUDRATE)

push:
	 /home/holla/arduino-1.6.5/hardware/tools/avr/bin/avrdude -C/home/holla/arduino-1.6.5/hardware/tools/avr/etc/avrdude.conf -v -patmega328p -carduino  -P net:192.168.0.35:2000 -D -U flash:w:build-cli/arduino.hex:i
