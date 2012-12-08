#include <stdint.h>
#include "WarmDirt.h"
#include "PID_v1.h"

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

#define LIDUPTHRESHOLD              44.0 
#define LIDDOWNTHRESHOLD            42.5
#define LIDMOVINGTIME               400000L
#define LIDSTATEDOWN                'd'
#define LIDSTATEMOVINGDOWN          'v'
#define LIDSTATEMOVINGUP            '^'
#define LIDSTATEUP                  'u'
uint8_t lidstate;
uint32_t lidMovingTimeout;

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
    lidstate   = LIDSTATEDOWN;
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
        case 'D':
            speedB = -100;
            speedB = wd.motorBSpeed(speedB);
            lidMovingTimeout = millis() + LIDMOVINGTIME;
            lidstate = LIDSTATEMOVINGDOWN;
            break;
        case 'U':
            speedB = 100;
            speedB = wd.motorBSpeed(speedB);
            lidMovingTimeout = millis() + LIDMOVINGTIME;
            lidstate = LIDSTATEMOVINGUP;
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

        switch (lidstate) {
            case LIDSTATEDOWN:
                Serial.print("closed");
                break;
            case LIDSTATEMOVINGUP:
                Serial.print("opening");
                break;
            case LIDSTATEUP:
                Serial.print("open");
                break;
            case LIDSTATEMOVINGDOWN:
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

void lidLoop() {
    double be  = wd.getBoxExteriorTemperature();
    switch (lidstate) {
        case LIDSTATEDOWN:
            if ( be > LIDUPTHRESHOLD ) {
                speedB = 100;
                speedB = wd.motorBSpeed(speedB);
                lidMovingTimeout = millis() + LIDMOVINGTIME;
                lidstate = LIDSTATEMOVINGUP;
            }
            break;
        case LIDSTATEMOVINGUP:
            if (millis() > lidMovingTimeout) {
                speedB = 0;
                speedB = wd.motorBSpeed(speedB);
                lidstate = LIDSTATEUP;
            }
            break;
        case LIDSTATEUP:
            if ( be < LIDDOWNTHRESHOLD ) {
                speedB = -100;
                speedB = wd.motorBSpeed(speedB);
                lidMovingTimeout = millis() + LIDMOVINGTIME;
                lidstate = LIDSTATEMOVINGDOWN;
            }
            break;
        case LIDSTATEMOVINGDOWN:
            if (millis() > lidMovingTimeout) {
                speedB = 0;
                speedB = wd.motorBSpeed(speedB);
                lidstate = LIDSTATEDOWN;
            }
            break;
    }
}

void loop() {
    statusLoop();
    commLoop();
    lightLoop();
    lidLoop();
}
