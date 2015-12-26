#define pinTrig 2
#define pinEcho 3
#define AVECOUNT 10

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

void setup() {
  Serial.begin(57600);
  pinMode(pinTrig, OUTPUT);
  pinMode(pinEcho, INPUT);
}

void loop() {
  long d;
  d = getDurationAve();
  Serial.println(d);
}
