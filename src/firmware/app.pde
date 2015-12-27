#include <stdint.h>
#include "WarmDirt.h"
#include "PID_v1.h"

/*
/home/holla/arduino-1.6.5/hardware/tools/avr/bin/avrdude -C/home/holla/arduino-1.6.5/hardware/tools/avr/etc/avrdude.conf -v -patmega328p -carduino  -P net:192.168.0.35:2000 -D -U flash:w:build-cli/arduino.hex:i

echo 'R' | nc 192.168.0.35 2000; /home/holla/arduino-1.6.5/hardware/tools/avr/bin/avrdude -C/home/holla/arduino-1.6.5/hardware/tools/avr/etc/avrdude.conf -v -patmega328p -carduino  -P net:192.168.0.35:2000 -D -U flash:w:build-cli/arduino.hex:i

/home/holla/arduino-1.6.5/hardware/tools/avr/bin/avrdude -C/home/holla/arduino-1.6.5/hardware/tools/avr/etc/avrdude.conf -v -patmega328p -carduino -P/dev/ttyUSB0 -b57600 -D -U flash:w:build-cli/arduino.hex:i

echo 'R' | nc 192.168.0.35 2000;avrdude -q -V -p atmega328p -c stk500v1 -P net:192.168.0.35:2000 -U flash:w:build-cli/arduino.hex:i
*/

#include "Time.h"
#define TIME_MSG_LEN  11

WarmDirt wd;

/* notes
    uses Time functions from arduino, http://playground.arduino.cc/Code/Time 
    T1354928747
    TZ_adjust=-7;  echo T$(($(date +%s)+60*60*$TZ_adjust)) 
*/

#define pinTrig 2
#define pinEcho 3

#define pinOverride  9
#define pinUp       A4
#define pinDown     A5
#define pinUpLimit  11

#define VPLUSR1     59000
#define VPLUSR2     179000
#define VPLUSSCALE  0.019697    // (((5.0/1024.0)/VPLUSR2)*(VPLUSR1+VPLUSR2))

#define STATUSUPDATEINVTERVAL   1000
#define ACTIVITYUPDATEINVTERVAL 500

int sunlightstate;
#define SUNLIGHTTHRESHOLD              500
#define STATESUNLIGHTABOVETHRESHOLD    'L'
#define STATESUNLIGHTBELOWTHRESHOLD    'D'

#define LIGHTLEVELDAY                50
#define LIGHTLEVELNIGHT              20

#define DOORUPMOVINGTIME             30000L
#define DOORDOWNMOVINGTIME           25000L
#define DOOR_STATE_OPENING          '^'
#define DOOR_STATE_CLOSING          'v'
#define DOOR_STATE_OPEN             'o'
#define DOOR_STATE_CLOSED           'c'
#define DOOR_STATE_MANUAL           'm'
#define DOOR_FULLY_OPEN_DISTANCE    23.7
#define DOOR_OPEN_SET               18.0
#define DOOR_CLOSE_SET              0.3
uint8_t doorState;
#define AVECOUNT                    10
double  doorKp            = 200;
double  doorKi            = 0.1;
double  doorKd            = 0.0;
double  doorSetPoint;
double  doorPosition;
double  doorMotorSpeed;

PID doorPID(&doorPosition, &doorMotorSpeed, &doorSetPoint, doorKp, doorKi, doorKd, DIRECT);


uint32_t timeoutLightOff;
uint32_t correlationID;
#define  LIGHTONTIME                10800000L   // 3*60*60*1000

uint8_t  mode;
#define  MODE_AUTO     'A'
#define  MODE_MANUAL   'M'
#define  MODE_OVERRIDE 'O'
uint32_t timeoutMode;

#define LIGHTONLEVEL 50

uint32_t lightUpdate;

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

void doorOpen() {
  doorSetPoint = DOOR_OPEN_SET;
}

void doorClose() {
  doorSetPoint = DOOR_CLOSE_SET;
}

uint8_t lightSwitch() {
  return wd.adcaverage(PINHEATEDDIRT,SAMPLES) < 500;
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

uint32_t nextIdleStatusUpdate;
uint32_t nextActivityUpdate;

int8_t speedA = 0;
int8_t speedB = 0;


void reset() {
    asm volatile ("jmp 0x3E00");        /* reset vector for 328P at this bootloader size */
}

void fullStop() {
    speedB = 0;
    wd.motorBSpeed(speedB);
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

long getDuration() {
    long duration, distance;
    digitalWrite(pinTrig, LOW);
    delayMicroseconds(2);
    digitalWrite(pinTrig, HIGH);
    delayMicroseconds(5);
    digitalWrite(pinTrig, LOW);
    duration = pulseIn(pinEcho, HIGH, 4000);
    // Serial.println(duration);
    return duration;
}

long getDurationAve() {
    long sum = 0;

    for (int i = 0; i < AVECOUNT; i++) {
      sum += getDuration();
      delay(25);
    }
    sum = sum / AVECOUNT;
    return sum;
}

float getDistance() {
    float d;
    long duration = getDurationAve();
    d = microsecondsToInches(duration);
    d = DOOR_FULLY_OPEN_DISTANCE - d;
    //Serial.println(d);
    return d;
}

void setup() {
    float d;
    Serial.begin(57600);
    Serial.println("WarmChicken begin");
    doorState = DOOR_STATE_CLOSED;
    sunlightstate = STATESUNLIGHTABOVETHRESHOLD;
    doorSetPoint = 1.0;
    doorPID.SetMode(AUTOMATIC);
    doorPID.SetOutputLimits(-100,100);

    timeoutLightOff = 0;
    correlationID = 0;

    pinMode(pinTrig, OUTPUT);
    pinMode(pinEcho, INPUT);

    pinMode(pinOverride, INPUT);
    pinMode(pinUp, INPUT);
    pinMode(pinUpLimit, INPUT);
    pinMode(pinDown, INPUT);

    mode = MODE_AUTO;
}

void lightOnRamp() {
    int i;
    for (i = 0; i < 101; i++) {
        speedA = i;
        speedA = wd.motorASpeed(speedA);
        delay(100);
    }
}

void lightOn(int v) {
    speedA = v;
    speedA = wd.motorASpeed(speedA);
}

void lightOff() {
    speedA = 0;
    speedA = wd.motorASpeed(speedA);
}

void lightOffRamp() {
    int i;
    for (i = 100; i >= 0; i--) {
        speedA = i;
        speedA = wd.motorASpeed(speedA);
        delay(100);
    }
}

void commProcess(int c) {
    switch (c) {
    case 's':
        nextIdleStatusUpdate = 0;
        break;
    case 'L':
        Serial.println("light on");
        lightOn(LIGHTONLEVEL);
        break;
    case 'f':
        Serial.println("light off");
        lightOff();
        break;
    case 'R':
        Serial.println("reset");
        delay(100);
        reset();
        break;
    case 'd':
        Serial.println("distance");
        Serial.println(getDistance());
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
    case 'j':
        speedA += MOTORSPEEDINC;
        speedA = wd.motorASpeed(speedA);
        Serial.print("a = ");
        Serial.println(speedA);
        break;
    case 'I':
        doorSetPoint += 0.5;
        break;
    case 'K':
        doorSetPoint -= 0.5;
        break;
    case 'l':
        speedA -= MOTORSPEEDINC;
        speedA = wd.motorASpeed(speedA);
        Serial.print("a = ");
        Serial.println(speedA);
        break;
    case ' ':
        Serial.println("full stop");
        speedA = 0;
        speedB = 0;
        wd.motorASpeed(speedA);
        wd.motorBSpeed(speedB);
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
    case 'A': // MODE_AUTO
        Serial.println("auto");
        fullStop();
        timeoutMode = 0;
        mode = MODE_AUTO;
        break;
    case 'M': // MODE_MANUAL
        Serial.println("manual");
        fullStop();
        timeoutMode = millis() + 30000;
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

void commLoop() {
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

    v = wd.getBoxInteriorTemperature() - 10.0;  // why - 10?
    Serial.print(v, 2);
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
    case DOOR_STATE_OPEN:
        Serial.print("open");
        break;
    case DOOR_STATE_CLOSING:
        Serial.print("closing");
        break;
    case DOOR_STATE_MANUAL:
        Serial.print("manual");
        break;
    }

    Serial.print("|");
    Serial.print(speedA, DEC);
    Serial.print("|");
    Serial.print((char)sunlightstate);
    Serial.print("|");
    Serial.print(getDistance());
    Serial.println();
}

void printStatusJSON() {
    double v;

/*
    Serial.print("{\"time\":\"");
    timePrint();
    Serial.print("\"");
*/
    Serial.print("{\"correlationID\":");
    Serial.print(correlationID++);

    Serial.print(",\"uptime\":");
    Serial.print((unsigned long)(millis()/1000));

    Serial.print(",\"temperatureInterior\":");
    v = wd.getBoxInteriorTemperature() - 10.0;  // why - 10?
    Serial.print(v, 0);
    Serial.print("");

    Serial.print(",\"temperatureExterior\":");
    v = wd.getBoxExteriorTemperature() - 10.0;
    Serial.print(v, 0);
    Serial.print("");

    Serial.print(",\"lightLevelExterior\":");
    v = wd.getLightSensor();
    Serial.print((int)v);
    Serial.print("");

    Serial.print(",\"lightLevelInterior\":");
    Serial.print(speedA, DEC);
    Serial.print("");

    Serial.print(",\"batteryVoltage\":\"");
    v = getVPlus();
    Serial.print(v, 2);
    Serial.print("\"");

    Serial.print(",\"mode\":\"");
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

    Serial.print(",\"overridePosition\":\"");
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

    Serial.print(",\"upperLimitSwitch\":");
    if (getUpLimit()) {
        Serial.print(1);
    } else {
        Serial.print(0);
    }

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
    case DOOR_STATE_OPEN:
        Serial.print("open");
        break;
    case DOOR_STATE_CLOSING:
        Serial.print("closing");
        break;
    case DOOR_STATE_MANUAL:
        Serial.print("manual");
        break;
    }
    Serial.print("\"");

    Serial.print(",\"lightSwitch\":");
        if (lightSwitch()) {
            Serial.print(1);
        } else {
            Serial.print(0);
        }

    Serial.print(",\"doorPosition\":");
    Serial.print(doorPosition,1);

    Serial.print(",\"doorSetPoint\":");
    Serial.print(doorSetPoint,1);


    Serial.println("}");
}

void loopStatus() {
    char buffer[30];
    uint32_t now = millis();
    int i;

    if (now > nextActivityUpdate) {
        wd.activityToggle();
        nextActivityUpdate = now + ACTIVITYUPDATEINVTERVAL;
    }

    if (now > nextIdleStatusUpdate) {
        printStatusJSON();
        nextIdleStatusUpdate = millis() + STATUSUPDATEINVTERVAL;
    }
}

void loopLight() {
    if (lightSwitch()) {
      lightOn(100);
    } else {
      if (speedA != LIGHTONLEVEL) {
        lightOff();
      }
    }

    if (doorState == DOOR_STATE_OPENING) {
        lightOff();
    }
      

/*
    int l = wd.getLightSensor();

    if (l > (SUNLIGHTTHRESHOLD + 50)) {
      timeoutLightOff = 0;  // this is kinda sloppy, should use a state var here
    }

    if ((l < SUNLIGHTTHRESHOLD) && (timeoutLightOff == 0)) {
       timeoutLightOff = millis() + LIGHTONTIME;
       lightOn();
    }
    if (millis() > timeoutLightOff) {
        lightOff();
    }
*/
}

/*
void loopTime() {
    if ((hour() == 6) && (minute() == 0) && (second() == 0)) {
        lightOn();
    }
    if ((hour() == 8) && (minute() == 0) && (second() == 0)) {
        lightOff();
    }
    if ((hour() == 16) && (minute() == 11) && (second() == 0)) {
        lightOn();
    }
    if ((hour() == 20) && (minute() == 0) && (second() == 0)) {
        lightOff();
    }
}
*/

void loopManual() {
    speedB = 0;

    if (getUp() && (!getUpLimit())) {
        speedB = 100;
    }
    if (getDown()) {
        speedB = -100;
    }
    speedB = wd.motorBSpeed(speedB);
}

void loopDoor() {
    int i, l = wd.getLightSensor();

    if (l > LIGHTLEVELDAY) {
      doorOpen();
    } else {
        if (l < LIGHTLEVELNIGHT) {
          doorClose();
        }
    }

  if (doorPosition > 0.00) {
    if (abs(doorPosition - doorSetPoint) > 0.30) {
      doorPID.Compute();
    } else {
      doorMotorSpeed = 0;
      doorPID.Initialize();
    }
  }

  if (!getUpLimit()) {
    speedB = wd.motorBSpeed(int(doorMotorSpeed));
  }


  if ((doorPosition > DOOR_CLOSE_SET) && (doorPosition < DOOR_OPEN_SET)) {
    if (speedB > 0) {
      doorState = DOOR_STATE_OPENING;
    }
    if (speedB < 0) {
      doorState = DOOR_STATE_CLOSING;
    }
  }
  if (doorPosition > (DOOR_OPEN_SET - 0.5)) {
    doorState = DOOR_STATE_OPEN;
  } 
  if (doorPosition < (DOOR_CLOSE_SET + 0.5)) {
    doorState = DOOR_STATE_CLOSED;
  } 
    
}

void loop() {
    loopStatus();
    commLoop();
    loopLight();
    doorPosition = getDistance();

    switch (mode) { 
      case MODE_AUTO:
        if (getUpLimit()) {
            fullStop();
        } else {
            loopDoor();
        }
        if (digitalRead(pinOverride)) {
          mode = MODE_OVERRIDE;
        }
        break;
      case MODE_MANUAL:
        if (millis() > timeoutMode) {
          mode = MODE_AUTO;
        }
        break;
      case MODE_OVERRIDE:
        if (!digitalRead(pinOverride)) {
          mode = MODE_AUTO;
        } 
        loopManual();
    }
    
}

