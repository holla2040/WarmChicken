#include <stdint.h>
#include "WarmDirt.h"

#define STATUSUPDATEINVTERVAL   10000
#define ACTIVITYUPDATEINVTERVAL 500

int lightstate;
#define LIGHTONDURATION             72000L
#define LIGHTTHRESHOLD              500
#define STATELIGHTABOVETHRESHOLD    'a'
#define STATELIGHTON                '1'
#define STATELIGHTOFF               '0'
#define STATELIGHTTEMPON            't'
#define STATELIGHTTEMPONDURATION    600000L

#define DOOROPENTHRESHOLD            200
#define DOORCLOSEHRESHOLD            800
#define DOORMOVINGTIME               30000L
#define DOOR_STATE_OPENING          '^'
#define DOOR_STATE_CLOSING          'v'
#define DOOR_STATE_OPEN             'o'
#define DOOR_STATE_CLOSED           'c'
uint8_t doorState;
uint32_t timeoutDoorMoving;

uint32_t lightUpdate;

char *ftoa(char *a, double f, int precision) {
  long p[] = {0,10,100,1000,10000,100000,1000000,10000000,100000000};

  char *ret = a;
  long heiltal = (long)f;
  itoa(heiltal, a, 10);
  while (*a != '\0') a++;
  *a++ = '.';
  long decimal = abs((long)((f - heiltal) * p[precision]));
  itoa(decimal, a, 10);
  return ret;
}

uint32_t nextIdleStatusUpdate;
uint32_t nextActivityUpdate;

int8_t   speedA = 0;
int8_t   speedB = 0;

WarmDirt wd;

void reset() {
    asm volatile("jmp 0x3E00"); /* reset vector for 328P at this bootloader size */
}


void setup() {                
    Serial.begin(57600);
    Serial.println("WarmChicken begin");
    lightstate = STATELIGHTOFF;
    doorState   = DOOR_STATE_CLOSED;
}


void commProcess(int c) {
    switch (c) {
        case 's':
            nextIdleStatusUpdate = 0;
            break;
        case 'R':
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
                        lightstate = STATELIGHTOFF;
                    }
                    if (c == '1') {
                        wd.load1On();
                        lightstate = STATELIGHTTEMPON;
                        lightUpdate = millis() + STATELIGHTTEMPONDURATION;
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
        case '0': // avrdude sends 0-space 
            while (!Serial.available()) ;
            c = Serial.read();
            if (c == ' ') {
                reset();
            }
            break;
        case 'O':
            speedB = -100;
            speedB = wd.motorBSpeed(speedB);
            timeoutDoorMoving = millis() + DOORMOVINGTIME;
            doorState = DOOR_STATE_OPENING;
            break;
        case 'C':
            speedB = 100;
            speedB = wd.motorBSpeed(speedB);
            timeoutDoorMoving = millis() + DOORMOVINGTIME;
            doorState = DOOR_STATE_CLOSING;
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

void statusLoop() {
    char buffer[30];
    uint32_t now = millis();
    double v;
    int i;
    if (now > nextActivityUpdate) {
        wd.activityToggle();
        nextActivityUpdate = now + ACTIVITYUPDATEINVTERVAL;
    }

    if (now > nextIdleStatusUpdate) {
        Serial.print(now);
        Serial.print("|");

        v  = wd.getBoxInteriorTemperature();
        Serial.print(v,2);
        Serial.print("|");

        v  = wd.getBoxExteriorTemperature();
        Serial.print(v,2);
        Serial.print("|");

        v  = wd.getLightSensor();
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
        }

        Serial.print("|");
        i = wd.getLoad0On();
        Serial.print(i,DEC);
        Serial.print("|");

        switch (lightstate) {
            case STATELIGHTON:
                sprintf(buffer,"timed on %ds",(lightUpdate - millis())/1000);
                break;
            case STATELIGHTOFF:
                sprintf(buffer,"off");
                break;
            case STATELIGHTTEMPON:
                sprintf(buffer,"temp on %ds",(lightUpdate - millis())/1000);
                break;
            case STATELIGHTABOVETHRESHOLD:
                sprintf(buffer,"sunlight %d",wd.getLightSensor());
                break;
        }
        Serial.print(buffer);


        Serial.println();

        nextIdleStatusUpdate = millis() + STATUSUPDATEINVTERVAL;
    }
}

void lightLoop() {
    int l = wd.getLightSensor();
    if (l > (LIGHTTHRESHOLD + 150)) { // 150 is larger than light contribution
        lightstate = STATELIGHTABOVETHRESHOLD;
        wd.load1Off();
        return;
    }
    if (lightstate == STATELIGHTABOVETHRESHOLD) {
        if (l < LIGHTTHRESHOLD) {
            wd.load1On();
            lightstate = STATELIGHTON;
            lightUpdate = millis() + LIGHTONDURATION;
        }
    }
    if (lightstate == STATELIGHTON || lightstate == STATELIGHTTEMPON) {
        if (millis() > lightUpdate) {
            wd.load1Off();
            lightstate = STATELIGHTOFF;
        }
    }
}

void doorLoop() {
    int l = wd.getLightSensor();
    switch (doorState) {
        case DOOR_STATE_CLOSED:
            if ( l > DOOROPENTHRESHOLD ) {
                speedB = 100;
                speedB = wd.motorBSpeed(speedB);
                timeoutDoorMoving = millis() + DOORMOVINGTIME;
                doorState = DOOR_STATE_OPENING;
            }
            break;
        case DOOR_STATE_OPENING:
            if (millis() > timeoutDoorMoving) {
                speedB = 0;
                speedB = wd.motorBSpeed(speedB);
                doorState = DOOR_STATE_OPEN;
            }
            break;
        case DOOR_STATE_OPEN:
/*
            if ( l < DOORCLOSETHRESHOLD ) {
                speedB = -100;
                speedB = wd.motorBSpeed(speedB);
                timeoutDoorMoving = millis() + DOORMOVINGTIME;
                doorState = DOOR_STATE_CLOSING;
            }
*/
            break;
        case DOOR_STATE_CLOSING:
            if (millis() > timeoutDoorMoving) {
                speedB = 0;
                speedB = wd.motorBSpeed(speedB);
                doorState = DOOR_STATE_CLOSED;
            }
            break;
    }
}

void loop() {
    statusLoop();
    commLoop();
    lightLoop();
    doorLoop();
}
