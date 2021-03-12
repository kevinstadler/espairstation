#include <MD_MAX72xx.h>
MD_MAX72XX matrix(MD_MAX72XX::FC16_HW, 15, 4);

volatile unsigned long lastMotion[3];

// I never actually managed to get change interrupts to run on 3 pins at the same
// time while also driving the MAX7219 LED over SPI (D1 and D3 always worked,
// D2, D6 all stayed on the same high/low)
#define PIR1 5
#define PIR2 12
#define PIR3 0

bool pinState(int pin) {
  return digitalRead(pin) == HIGH;
}

void ICACHE_RAM_ATTR motionDetectorInterrupt1() {
  bool state = pinState(PIR1);
  for (byte i = 0; i < 8; i++) {
    matrix.setColumn(i, state*255);
  }
  if (!state) {
    matrix.setChar(5, 48 + (millis() - lastMotion[0]) / 1000);
  }
  lastMotion[0] = millis();
}
void ICACHE_RAM_ATTR motionDetectorInterrupt2() {
  bool state = pinState(PIR2);
  for (byte i = 8; i < 16; i++) {
    matrix.setColumn(i, state*255);
  }
  if (!state) {
    matrix.setChar(13, 48 + (millis() - lastMotion[1]) / 1000);
  }
  lastMotion[1] = millis();
}
void ICACHE_RAM_ATTR motionDetectorInterrupt3() {
bool state = pinState(PIR3);
  for (byte i = 16; i < 24; i++) {
    matrix.setColumn(i, state*255);
  }
  if (!state) {
    matrix.setChar(21, 48 + (millis() - lastMotion[2]) / 1000);
  }
  lastMotion[2] = millis();
}

void setup() {
  pinMode(PIR1, INPUT);
  pinMode(PIR2, INPUT);
  pinMode(PIR3, INPUT);
//  Serial.begin(115200);
  matrix.begin();
//  Serial.println("Current pin states:");
  motionDetectorInterrupt1();
  motionDetectorInterrupt2();
  motionDetectorInterrupt3();
//  Serial.print("Attaching motion detector interrupt...");
  attachInterrupt(digitalPinToInterrupt(PIR1), motionDetectorInterrupt1, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIR2), motionDetectorInterrupt2, CHANGE);
  attachInterrupt(digitalPinToInterrupt(PIR3), motionDetectorInterrupt3, CHANGE);
//  Serial.println("setup finished.");
}


void loop() {
  delay(10000);
}
