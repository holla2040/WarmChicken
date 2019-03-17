// this code is serious need of refactor
#include <stdint.h>
#include "WarmDirt.h"

/*
/home/holla/arduino-1.6.5/hardware/tools/avr/bin/avrdude -C/home/holla/arduino-1.6.5/hardware/tools/avr/etc/avrdude.conf -v -patmega328p -carduino  -P net:192.168.0.35:23 -D -U flash:w:build-cli/arduino.hex:i

echo 'R' | nc 192.168.0.35 23; /home/holla/arduino-1.6.5/hardware/tools/avr/bin/avrdude -C/home/holla/arduino-1.6.5/hardware/tools/avr/etc/avrdude.conf -v -patmega328p -carduino  -P net:192.168.0.35:23 -D -U flash:w:build-cli/arduino.hex:i

/home/holla/arduino-1.6.5/hardware/tools/avr/bin/avrdude -C/home/holla/arduino-1.6.5/hardware/tools/avr/etc/avrdude.conf -v -patmega328p -carduino -P/dev/ttyUSB0 -b57600 -D -U flash:w:build-cli/arduino.hex:i

echo 'R' | nc 192.168.0.41 23;avrdude -q -V -p atmega328p -c stk500v1 -P net:192.168.0.41:23 -U flash:w:build-cli/arduino.hex:i
*/

#include "Time.h"
#define TIME_MSG_LEN  11

WarmDirt wd;

/* notes
    uses Time functions from arduino, http://playground.arduino.cc/Code/Time 
    T1354928747
    TZ_adjust=-7;  echo T$(($(date +%s)+60*60*$TZ_adjust)) 
*/

#define pinOverride  9
#define pinUp       A4
#define pinDown     A5
#define pinUpLimit  11

#define pinDoorOpen   2
#define pinDoorClosed 3

#define VPLUSSCALE  0.019697    // (((5.0/1024.0)/VPLUSR2)*(VPLUSR1+VPLUSR2))

#define STATUSUPDATEINVTERVAL   1000
#define ACTIVITYUPDATEINVTERVAL 500
#define RADIOREBOOTINVTERVAL    3600000L

int sunlightstate;
#define STATESUNLIGHTABOVETHRESHOLD    'L'
#define STATESUNLIGHTBELOWTHRESHOLD    'D'

#define MOTORRUNTIMEMAX 60

#define LIGHTLEVELDAY                200
#define LIGHTLEVELNIGHT              40

#define HEATERTHRESHOLD             20
#define BADSENSORTHRESHOLD          20

#define DOORUPMOVINGTIME             30000L
#define DOORDOWNMOVINGTIME           25000L
#define DOOR_STATE_OPENING          '^'
#define DOOR_STATE_CLOSING          'v'
#define DOOR_STATE_OPEN             'o'
#define DOOR_STATE_CLOSED           'c'
#define DOOR_STATE_MANUAL           'm'
#define DOOR_STATE_STOPPED          's'
#define DOOR_STATE_LIMIT            'l'
#define DOOR_STATE_MOTORRUNTIME     'R'
#define DOOR_FULLY_OPEN_DISTANCE    24.2
#define DOOR_OPEN_SET               18.0
#define DOOR_CLOSE_SET              0.5
uint8_t doorState;
#define AVECOUNT                    10
double  doorMotorSpeed;

uint32_t timeoutLightOff;
uint32_t correlationID;

uint8_t  mode;
#define  MODE_AUTO     'A'
#define  MODE_MANUAL   'M'
#define  MODE_OVERRIDE 'O'

#define LIGHTONLEVEL 50

float temperatureInterior;

uint32_t nextIdleStatusUpdate;
uint32_t nextActivityUpdate;
uint32_t nextRadioReboot;

int8_t speedA = 0;
int8_t speedB = 0;

int motorRuntime;
uint32_t motorStartTime;

void printDigits(int digits) {
    // utility function for digital clock display: prints preceding colon and leading 0
    if (digits < 10)
        Serial.print('0');
    Serial.print(digits);
}

void timePrint() {
    printDigits(hour());
    Serial.print(":");
    printDigits(minute());
    Serial.print(":");
    printDigits(second());
}


uint8_t doorPositionOpen() {
  /* low is open */
  return !digitalRead(pinDoorOpen); 
}

uint8_t doorPositionClosed() {
  /* high is closed */
  return digitalRead(pinDoorClosed); 
}

uint8_t lightSwitch() {
  return wd.adcaverage(PINHEATEDDIRT,SAMPLES) < 500;
}

void doorSpeedSet(int speed) {
    if ((speedB == 0) && (speed != 0)) {
      motorStartTime = millis();
    }
    if (speed > 0) {
      doorState = DOOR_STATE_OPENING;
    } 
    if (speed < 0) {
      doorState = DOOR_STATE_CLOSING;
    } 
    speedB = wd.motorBSpeed(speed);
}


char *ftoa(char *a, double f, int precision) {
    long p[] = { 0, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000 };

    char *ret = a;
    long heiltal = (long)f;
    itoa(heiltal, a, 10);
    while (*a != '\0')
        a++;
    *a++ = '.';
    long decimal = abs((long)((f - heiltal) * p[precision]));
    itoa(decimal, a, 10);
    return ret;
}

void doorOpen() {
  if (!doorPositionOpen()) {
      doorSpeedSet(100);
      doorState = DOOR_STATE_OPENING;
  }
}

void doorClose() {
  if (!doorPositionClosed()) {
      doorSpeedSet(-50);
      doorState = DOOR_STATE_CLOSING;
  }
}

void reset() {
    asm volatile ("jmp 0x3E00");        /* reset vector for 328P at this bootloader size */
}

void fullStop() {
    doorSpeedSet(0);
    doorState = DOOR_STATE_STOPPED;
    if (doorPositionOpen()) {
      doorState = DOOR_STATE_OPEN;
    } 
    if (doorPositionClosed()) {
      doorState = DOOR_STATE_CLOSED;
    }
}

bool getUp() {
    return !digitalRead(pinUp);
}

bool getUpLimit() {
    return digitalRead(pinUpLimit);
}

bool getDown() {
    return !digitalRead(pinDown);
}

float getVPlus() {
    return wd.adcaverage(PINHEATEDDIRT, 5) * VPLUSSCALE;
}


float microsecondsToInches(long microseconds) {
    // According to Parallax's datasheet for the PING))), there are
    // 73.746 microseconds per inch (i.e. sound travels at 1130 feet per
    // second).  This gives the distance travelled by the ping, outbound
    // and return, so we divide by 2 to get the distance of the obstacle.
    // See: http://www.parallax.com/dl/docs/prod/acc/28015-PING-v1.3.pdf
    return microseconds / 73.746 / 2.0;
    // return microseconds / 74 / 2;
}

float microsecondsToCentimeters(long microseconds) {
    // The speed of sound is 340 m/s or 29 microseconds per centimeter.
    // The ping travels out and back, so to find the distance of the
    // object we take half of the distance travelled.
    return microseconds / 29.4 / 2.0;
    //return microseconds / 29 / 2;
}

void handleReset() {
    delay(200);
    if (Serial.available() == 4)
      if (Serial.read() == 'e')
        if (Serial.read() == 's')
          if (Serial.read() == 'e')
            if (Serial.read() == 't') reset();
}

void setup() {
    Serial.begin(57600);
    Serial.println("WarmChicken begin");

    pinMode(pinDoorOpen, INPUT);
    digitalWrite(pinDoorOpen, HIGH); // enable pull up
    pinMode(pinDoorClosed, INPUT);
    digitalWrite(pinDoorClosed, HIGH); // enable pull up

    pinMode(pinOverride, INPUT);
    pinMode(pinUp, INPUT);
    pinMode(pinUpLimit, INPUT);
    pinMode(pinDown, INPUT);

    mode = MODE_AUTO;
    doorState = DOOR_STATE_STOPPED;
    sunlightstate = STATESUNLIGHTABOVETHRESHOLD;
    nextRadioReboot = millis() + RADIOREBOOTINVTERVAL;

    timeoutLightOff = 0;
    correlationID = 0;
    motorRuntime = 0;

}

void lightOnRamp() {
    int i;
    for (i = 0; i < 101; i++) {
        speedA = wd.motorASpeed(i);
        delay(25);
    }
}

void lightOn(int v) {
    speedA = wd.motorASpeed(v);
}

void lightOff() {
    speedA = wd.motorASpeed(0);
}

void lightToggle() {
  if (speedA) {
    speedA = 0;
  } else {
    speedA = LIGHTONLEVEL;
  }
  speedA = wd.motorASpeed(speedA);
} 

void lightOffRamp() {
    int i;
    for (i = 100; i >= 0; i--) {
        speedA = wd.motorASpeed(i);
        delay(100);
    }
}

void commProcess(int c) {
    switch (c) {
    case 's':
        nextIdleStatusUpdate = 0;
        break;
    case 'L':
        lightOnRamp();
        break;
    case 'F':
        lightOff();
        break;
    case 't':
        lightToggle();
        break;
    case 'Z':
        handleReset();
        break;
    case 'R':
        delay(100);
        reset();
        break;
    case 'a':
        Serial.print("a");
        while (!Serial.available()) ;
        c = Serial.read();
        Serial.print((char)c);
        if (c == '0') {
            while (!Serial.available()) ;
            c = Serial.read();
            Serial.print((char)c);
            if (c == '0') {
                wd.load0Off();
            }
            if (c == '1') {
                wd.load0On();
            }
        } else {
            if (c == '1') {
                while (!Serial.available()) ;
                c = Serial.read();
                Serial.print((char)c);
                if (c == '0') {
                    wd.load1Off();
                }
                if (c == '1') {
                    wd.load1On();
                }
            }
        }
        Serial.println();
        break;
    case 'i':
        speedB += MOTORSPEEDINC;
        speedB = wd.motorBSpeed(speedB);
        Serial.print("b = ");
        Serial.println(speedB);
        break;
    case 'k':
        speedB -= MOTORSPEEDINC;
        speedB = wd.motorBSpeed(speedB);
        Serial.print("b = ");
        Serial.println(speedB);
        break;
    case ' ':
        speedA = wd.motorASpeed(0);
        fullStop();
        wd.stepperDisable();
        break;
    case '0':                  // avrdude sends 0-space 
        while (!Serial.available()) ;
        c = Serial.read();
        if (c == ' ') {
            reset();
        }
        break;
    case 'O':
        Serial.println("door open");
        doorOpen();
        break;
    case 'C':
        Serial.println("door close");
        doorClose();
        break;
    case 'H':
        if (wd.getLoad0On()) {
          wd.load0Off();
        } else {
          wd.load0On();
        }
        break;
    case 'A': // MODE_AUTO
        Serial.println("auto");
        fullStop();
        mode = MODE_AUTO;
        break;
    case 'M': // MODE_MANUAL
        Serial.println("manual");
        fullStop();
        mode = MODE_MANUAL;
        break;
    case 'T':
        time_t pctime = 0;
        for (int i = 0; i < TIME_MSG_LEN; i++) {
            while (!Serial.available()) ;
            c = Serial.read();
            if (c >= '0' && c <= '9') {
                pctime = (10 * pctime) + (c - '0');     // convert digits to a number    
            }
        }
        //Serial.println(pctime,DEC);
        setTime(pctime);
        break;
    }
}

void loopComm() {
    int c;
    if (Serial.available()) {
        c = Serial.read();
        commProcess(c);
    }
}

void printStatusDelim() {
    double v;
    timePrint();
    Serial.print("|");

    temperatureInterior = wd.getBoxInteriorTemperature() - 10.0;  // why - 10?
    Serial.print(temperatureInterior, 2);
    Serial.print("|");

    v = wd.getBoxExteriorTemperature() - 10.0;
    Serial.print(v, 2);
    Serial.print("|");

    v = wd.getLightSensor();
    Serial.print((int)v);
    Serial.print("|");

    switch (doorState) {
      case DOOR_STATE_CLOSED:
          Serial.print("closed");
          break;
      case DOOR_STATE_OPENING:
          Serial.print("opening");
          break;
      case DOOR_STATE_STOPPED:
          Serial.print("stopped");
          break;
      case DOOR_STATE_OPEN:
          Serial.print("open");
          break;
      case DOOR_STATE_CLOSING:
          Serial.print("closing");
          break;
      case DOOR_STATE_MANUAL:
          Serial.print("manual");
          break;
      case DOOR_STATE_LIMIT:
          Serial.print("limit");
          break;
    }

    Serial.print("|");
    Serial.print(speedA, DEC);
    Serial.print("|");
    Serial.print((char)sunlightstate);
    Serial.println();
}

void printStatusJSON() {
    double v;

/*
    Serial.print("{\"time\":\"");
    timePrint();
    Serial.print("\"");
*/
    Serial.print("{\"systemCorrelationId\":");
    Serial.print(correlationID++);

    Serial.print(",\"systemUptime\":");
    Serial.print((unsigned long)(millis()/1000));

    Serial.print(",\"temperatureInterior\":");
    temperatureInterior = wd.getBoxInteriorTemperature() - 10.0;  // why - 10?
    Serial.print(temperatureInterior, 1);
    Serial.print("");

    Serial.print(",\"temperatureExterior\":");
    v = wd.getBoxExteriorTemperature() - 10.0;
    Serial.print(v, 1);
    Serial.print("");

    Serial.print(",\"lightLevelExterior\":");
    v = wd.getLightSensor();
    Serial.print((int)v);
    Serial.print("");

    Serial.print(",\"lightLevelInterior\":");
    Serial.print(speedA, DEC);
    Serial.print("");

    Serial.print(",\"batteryVoltage\":");
    v = getVPlus();
    Serial.print(v, 2);

    Serial.print(",\"systemMode\":\"");
    switch (mode) {
      case MODE_AUTO:
        Serial.print("auto");
        break;
      case MODE_MANUAL:
        Serial.print("manual");
        break;
      case MODE_OVERRIDE:
        Serial.print("override");
        break;
    }
    Serial.print("\"");



    Serial.print(",\"doorMotorSpeed\":");
    Serial.print(speedB);
    Serial.print("");

    Serial.print(",\"doorState\":\"");
    switch (doorState) {
      case DOOR_STATE_CLOSED:
          Serial.print("closed");
          break;
      case DOOR_STATE_OPENING:
          Serial.print("opening");
          break;
      case DOOR_STATE_STOPPED:
          Serial.print("stopped");
          break;
      case DOOR_STATE_OPEN:
          Serial.print("open");
          break;
      case DOOR_STATE_CLOSING:
          Serial.print("closing");
          break;
      case DOOR_STATE_MANUAL:
          Serial.print("manual");
          break;
      case DOOR_STATE_LIMIT:
          Serial.print("limit");
          break;
    }
    Serial.print("\"");

    Serial.print(",\"heaterPower\":");
    Serial.print((int)wd.getLoad0On());

    Serial.print(",\"switchLight\":");
    if (lightSwitch()) {
        Serial.print(1);
    } else {
        Serial.print(0);
    }

    Serial.print(",\"switchRunStop\":");
    if (digitalRead(pinOverride)) {
      Serial.print("\"override\"");
    } else {
      Serial.print("\"auto\"");
    }

    Serial.print(",\"switchJog\":\"");
    if (getUp()) {
        Serial.print("up");
    } else {
        if (getDown()) {
            Serial.print("down");
        } else {
            Serial.print("off");
        }
    }
    Serial.print("\"");

    Serial.print(",\"switchLimitUpper\":");
    if (getUpLimit()) {
        Serial.print(1);
    } else {
        Serial.print(0);
    }

    Serial.print(",\"switchDoorOpen\":");
    Serial.print((int)doorPositionOpen());
    Serial.print(",\"switchDoorClosed\":");
    Serial.print((int)doorPositionClosed());

    Serial.print(",\"doorMotorRuntime\":");
    Serial.print(motorRuntime);

    Serial.println("}");
}

void loopStatus() {
    uint32_t now = millis();

    if (now > nextActivityUpdate) {
        wd.activityToggle();
        nextActivityUpdate = now + ACTIVITYUPDATEINVTERVAL;
    }

    if (now > nextIdleStatusUpdate) {
        printStatusJSON();
        if (speedB) {
          nextIdleStatusUpdate = millis() + 500; // if door is moving, send 2 updates a sec
        } else {
          nextIdleStatusUpdate = millis() + STATUSUPDATEINVTERVAL;
        }
    }
}

int lastLightSwitch;
void loopLight() {
    int l;
    l = lightSwitch();
    if (l) {
      lightOn(100);
    } else {
      // light switch isn't on but last time was it on?
      if (l != lastLightSwitch){ 
        lightOff();
      }
    }

    if (mode == MODE_AUTO) {
      if (wd.getLightSensor() > (LIGHTLEVELDAY+100)) {
          lightOff();
      }
    }

    lastLightSwitch = l;
}

void loopOverride() {
    speedB = 0;

    if (getUp() && (!getUpLimit())) {
        doorSpeedSet(100);
    } else {
      if (getDown()) {
          doorSpeedSet(-100);
      } else {
        doorSpeedSet(0);
      }
    }
}


int lastUpLimit;
void loopDoor() {
  int lightLevel = wd.getLightSensor();

  // this happen regardless of mode, you hit the limit it run towards until upper limit
  if (getUpLimit()) {
    if (speedB > 0) {
      doorSpeedSet(-100);
    } else {
      if (speedB < 0) {
        doorSpeedSet(100);
      }
    }
    while (getUpLimit()) {}; // run until we get to door open switch
    fullStop();
  }

  if (doorState == DOOR_STATE_OPENING) {
    if (doorPositionOpen()) {
      fullStop();
      doorState = DOOR_STATE_OPEN;
    } 
  }

  if (doorState == DOOR_STATE_CLOSING) {
    if (doorPositionClosed()) {
      fullStop();
      doorState = DOOR_STATE_CLOSED;
    }
  }

  if (mode == MODE_AUTO) {
    if ((lightLevel > LIGHTLEVELDAY) && (doorState != DOOR_STATE_OPEN)) {
      // CH doorOpen();
    } else {
        if ((lightLevel < LIGHTLEVELNIGHT) && (doorState != DOOR_STATE_CLOSED)) {
          doorClose();
        }
    }
  }
}

void loopHeater() {
    if (mode == MODE_AUTO) {
      if ((temperatureInterior < HEATERTHRESHOLD) && (doorState == DOOR_STATE_CLOSED))  {
        wd.load0On();
      } else {
        wd.load0Off();
      }
  }
}

void loopMode() {
    switch (mode) { 
      case MODE_AUTO:
        if (digitalRead(pinOverride)) {
          mode = MODE_OVERRIDE;
        }
        break;
      case MODE_MANUAL:
        if (digitalRead(pinOverride)) {
          mode = MODE_OVERRIDE;
        }
        break;
      case MODE_OVERRIDE:
        if (!digitalRead(pinOverride)) {
          mode = MODE_AUTO;
        } 
        loopOverride();
    }
}

void loopMotor() {
  if (speedB) {
    motorRuntime = (millis() - motorStartTime) / 1000;
  }
  if (motorRuntime > MOTORRUNTIMEMAX) {
    fullStop();
  }
}

void loopRadio() {
  if (millis() > nextRadioReboot) {
    Serial.println("radio reboot");
    delay(1000);
    Serial.print("$$$");
    delay(1000);
    Serial.print("REBOOT\r");
    nextRadioReboot = millis() + RADIOREBOOTINVTERVAL;
    delay(5000);
  }
}

void loop() {
    loopMode();
    loopStatus();
    loopComm();
    loopDoor();
    loopHeater();
    loopLight();
    loopMotor();
    // loopRadio();
}

