#include <stdint.h>
#include "WarmDirt.h"

#include "Time.h"
#define TIME_MSG_LEN  11

/* notes
    uses Time functions from arduino, http://playground.arduino.cc/Code/Time 
    T1354928747
    TZ_adjust=-7;  echo T$(($(date +%s)+60*60*$TZ_adjust)) 
*/


#define STATUSUPDATEINVTERVAL   250
#define ACTIVITYUPDATEINVTERVAL 500

int sunlightstate;
#define SUNLIGHTTHRESHOLD              100
#define STATESUNLIGHTABOVETHRESHOLD    'L'
#define STATESUNLIGHTBELOWTHRESHOLD    'D'

#define DOOROPENTHRESHOLD            100
#define DOORCLOSEHRESHOLD            800
#define DOOROPENTEMPTHRESHOLD        20
#define DOORMOVINGTIME               120000L
#define DOOR_STATE_OPENING          '^'
#define DOOR_STATE_CLOSING          'v'
#define DOOR_STATE_OPEN             'o'
#define DOOR_STATE_CLOSED           'c'
uint8_t doorState;
uint32_t timeoutDoorMoving;

uint32_t lightUpdate;

void printDigits(int digits){
  // utility function for digital clock display: prints preceding colon and leading 0
  Serial.print(":");
  if(digits < 10)
    Serial.print('0');
  Serial.print(digits);
}

void timePrint(){
  Serial.print(hour());
  printDigits(minute());
  printDigits(second());
}


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
    doorState   = DOOR_STATE_CLOSED;
    sunlightstate = STATESUNLIGHTABOVETHRESHOLD;
}

void lightOnRamp() {
    int i;
    for (i = 0; i < 101; i++) {
        speedA = i;
        speedA = wd.motorASpeed(speedA);
        delay(100);
    }
}

void lightOn() {
    speedA = 100;
    speedA = wd.motorASpeed(speedA);
}

void lightOff() {
    int i;
    for (i = 100; i >= 0; i--) {
        speedA = i;
        speedA = wd.motorASpeed(speedA);
        delay(100);
    }
}

void doorOpen() {
    speedB = 100;
    speedB = wd.motorBSpeed(speedB);
    timeoutDoorMoving = millis() + DOORMOVINGTIME;
    doorState = DOOR_STATE_OPENING;
}


void commProcess(int c) {
    switch (c) {
        case 's':
            nextIdleStatusUpdate = 0;
            break;
        case 'L':
            lightOn();
            break;
        case 'K':
            lightOff();
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
        case 'l':
            speedA -= MOTORSPEEDINC; 
            speedA = wd.motorASpeed(speedA);
            Serial.print("a = ");
            Serial.println(speedA);
            break;
        case 'S':
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
            doorOpen();
            break;
        case 'C':
            speedB = -30;
            speedB = wd.motorBSpeed(speedB);
            timeoutDoorMoving = millis() + DOORMOVINGTIME;
            doorState = DOOR_STATE_CLOSING;
            break;
        case 'T':
            time_t pctime = 0;
            for(int i=0; i < TIME_MSG_LEN; i++){   
                while (!Serial.available());
                c = Serial.read();          
                if( c >= '0' && c <= '9'){   
                    pctime = (10 * pctime) + (c - '0') ; // convert digits to a number    
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

        timePrint();
        Serial.print("|");

        v  = wd.getBoxInteriorTemperature() - 10.0;
        Serial.print(v,2);
        Serial.print("|");

        v  = wd.getBoxExteriorTemperature() - 10.0;
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
        Serial.print(speedA,DEC);
        Serial.print("|");
        Serial.print((char)sunlightstate);
        Serial.println();

        nextIdleStatusUpdate = millis() + STATUSUPDATEINVTERVAL;
    }
}

void sunlightLoop() {
    int l = wd.getLightSensor();
    if (sunlightstate == STATESUNLIGHTBELOWTHRESHOLD) {
        if (l > (SUNLIGHTTHRESHOLD + 50)) { 
            doorOpen();
            sunlightstate = STATESUNLIGHTABOVETHRESHOLD;
        }
    }
    if (sunlightstate == STATESUNLIGHTABOVETHRESHOLD) {
        if (l < SUNLIGHTTHRESHOLD) {
            sunlightstate = STATESUNLIGHTBELOWTHRESHOLD;
        }
    }
}

void timeLoop() {
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

void doorLoop() {
    int l = wd.getLightSensor();
    float  v  = wd.getBoxExteriorTemperature() - 10.0;
    switch (doorState) {
        case DOOR_STATE_CLOSED:
            if ( (l > DOOROPENTHRESHOLD) && (v > DOOROPENTEMPTHRESHOLD) ) {
                doorOpen();
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
    sunlightLoop();
    doorLoop();
    timeLoop();
}
