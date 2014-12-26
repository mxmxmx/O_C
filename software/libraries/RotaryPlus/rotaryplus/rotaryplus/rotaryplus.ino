#include <rotaryplus.h>

Rotary r = Rotary(5, 6);

void checkRot() {
  r.process();
}

void setup() {
  Serial.begin(9600);
  attachInterrupt(5, checkRot, CHANGE);
  attachInterrupt(6, checkRot, CHANGE);
}

void loop() {
  if (r.change()) {
    Serial.println(r.pos);
  }
}

