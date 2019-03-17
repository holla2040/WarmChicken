# copy 2 arduino IDE arduino-builder lines to bin
# delete the unique numbers from build and cache directory, these are unique to IDE runs
# :s/_69751//g
# :s/_90469//g
# if you want quieter output, 
#  delete -dump-prefs -verbose
#  add -quiet=1
# hopefully arduino-cli will eliminate this hack
# or use, https://github.com/plerup/makeEspArduino
#
# passing 'name' argument to make
# make reboot name=esp8266

ino = $(wildcard *.ino)
name=wc
port=/dev/ttyUSB0
toolver=2.5.0-3-20ed2b9

bin:
	@mkdir -p /tmp/arduino_build /tmp/arduino_cache
	/home/holla/arduino/arduino-builder -dump-prefs -logger=machine -hardware /home/holla/arduino/hardware -hardware /home/holla/.arduino15/packages -hardware /home/holla/Arduino/hardware -tools /home/holla/arduino/tools-builder -tools /home/holla/arduino/hardware/tools/avr -tools /home/holla/.arduino15/packages -built-in-libraries /home/holla/arduino/libraries -libraries /home/holla/Arduino/libraries -fqbn=esp8266:esp8266:d1_mini:xtal=80,vt=flash,exception=disabled,eesz=4M1M,ip=lm2f,dbg=Disabled,lvl=None____,wipe=none,baud=921600 -ide-version=10808 -build-path /tmp/arduino_build -warnings=none -build-cache /tmp/arduino_cache -prefs=build.warn_data_percentage=75 -prefs=runtime.tools.mkspiffs.path=/home/holla/.arduino15/packages/esp8266/tools/mkspiffs/${toolver} -prefs=runtime.tools.mkspiffs-${toolver}.path=/home/holla/.arduino15/packages/esp8266/tools/mkspiffs/${toolver} -prefs=runtime.tools.esptool.path=/home/holla/.arduino15/packages/esp8266/tools/esptool/${toolver} -prefs=runtime.tools.esptool-${toolver}.path=/home/holla/.arduino15/packages/esp8266/tools/esptool/${toolver} -prefs=runtime.tools.xtensa-lx106-elf-gcc.path=/home/holla/.arduino15/packages/esp8266/tools/xtensa-lx106-elf-gcc/${toolver} -prefs=runtime.tools.xtensa-lx106-elf-gcc-${toolver}.path=/home/holla/.arduino15/packages/esp8266/tools/xtensa-lx106-elf-gcc/${toolver} -verbose ${ino}
	/home/holla/arduino/arduino-builder -compile -logger=machine -hardware /home/holla/arduino/hardware -hardware /home/holla/.arduino15/packages -hardware /home/holla/Arduino/hardware -tools /home/holla/arduino/tools-builder -tools /home/holla/arduino/hardware/tools/avr -tools /home/holla/.arduino15/packages -built-in-libraries /home/holla/arduino/libraries -libraries /home/holla/Arduino/libraries -fqbn=esp8266:esp8266:d1_mini:xtal=80,vt=flash,exception=disabled,eesz=4M1M,ip=lm2f,dbg=Disabled,lvl=None____,wipe=none,baud=921600 -ide-version=10808 -build-path /tmp/arduino_build -warnings=none -build-cache /tmp/arduino_cache -prefs=build.warn_data_percentage=75 -prefs=runtime.tools.mkspiffs.path=/home/holla/.arduino15/packages/esp8266/tools/mkspiffs/${toolver} -prefs=runtime.tools.mkspiffs-${toolver}.path=/home/holla/.arduino15/packages/esp8266/tools/mkspiffs/${toolver} -prefs=runtime.tools.esptool.path=/home/holla/.arduino15/packages/esp8266/tools/esptool/${toolver} -prefs=runtime.tools.esptool-${toolver}.path=/home/holla/.arduino15/packages/esp8266/tools/esptool/${toolver} -prefs=runtime.tools.xtensa-lx106-elf-gcc.path=/home/holla/.arduino15/packages/esp8266/tools/xtensa-lx106-elf-gcc/${toolver} -prefs=runtime.tools.xtensa-lx106-elf-gcc-${toolver}.path=/home/holla/.arduino15/packages/esp8266/tools/xtensa-lx106-elf-gcc/${toolver} -verbose ${ino}


flash: 
	@echo ${name}
	@echo "ota"
	curl --progress-bar -F "image=@/tmp/arduino_build/$(ino).bin" http://${name}/firmware | tee /dev/null

flashusb: 
	@- pkill -9 -f microcom
	/home/holla/.arduino15/packages/esp8266/tools/esptool/${toolver}/esptool -vv -cd nodemcu -cb 921600 -cp $(port) -ca 0x00000 -cf /tmp/arduino_build/$(ino).bin
	@- pkill -9 -f sleep


files:
	@./transferdata.sh
	@echo ${name}
	curl http://${name}/reload

ota:

reboot:
	@echo ${name}
	@echo "reboot"
	curl http://${name}/reboot

reset:
	@echo ${name}
	@echo "uploading reseting"
	curl http://${name}/reset

index:
	@echo ${name}
	@echo "uploading index.htm"
	@curl -F "file=@./data/index.htm" http://${name}/edit

style:
	@echo ${name}
	@echo "uploading style.css" 
	@curl -F "file=@./data/style.css" http://${name}/edit

config:
	@echo ${name}
	@echo "uploading config.json"
	@curl -F "file=@./data/config.json" http://${name}/edit
	@curl http://${name}/config.json

clean:
	rm -rf /tmp/arduino_build*
	rm -rf /tmp/arduino_cache*
