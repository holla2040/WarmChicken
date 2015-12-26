#include "PID_v1.h"
#include <Servo.h>

double Kp = 200.0, Ki = 0.1, Kd = 0.0;

#define pinTrig 2
#define pinEcho 3
#define AVECOUNT 10 

Servo myservo;

long getDuration() {
    long duration, distance;
    digitalWrite(pinTrig, LOW);
    delayMicroseconds(2);
    digitalWrite(pinTrig, HIGH);
    delayMicroseconds(5);
    digitalWrite(pinTrig, LOW);
    duration = pulseIn(pinEcho, HIGH);
    return duration;
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
    //Serial.println(d);
    return d;
}

double setPoint,doorPosition,motorSpeed;

PID doorPID(&doorPosition, &motorSpeed, &setPoint, Kp, Ki, Kd, DIRECT);

void setup()
{
  Serial.begin(57600);
  pinMode(pinTrig, OUTPUT);
  pinMode(pinEcho, INPUT);
  myservo.attach(9);  // attaches the servo on pin 9 to the servo object
  doorPID.SetMode(AUTOMATIC);
  doorPID.SetOutputLimits(-100,100);
  setPoint = 18.0;
}

void loop()
{
  doorPosition = getDistance();
  if (doorPosition > 0.00) {
    if (abs(doorPosition - setPoint) > 0.30) {
      doorPID.Compute();
    } else {
      doorPID.Initialize();
      motorSpeed = 0;
    }

    Serial.print(doorPosition,2);
    Serial.print(" ");
    Serial.print(motorSpeed,0);
    Serial.println();
  }

/*
  myservo.write(Input);
*/
  delay(100);
}
