volatile unsigned long lastMotion;

bool shouldDisplayBeOn() {
  return digitalRead(MOTIONSENSOR_PIN) == HIGH;
}

void ICACHE_RAM_ATTR motionDetectorInterrupt() {
  bool state = shouldDisplayBeOn();
  display.enableDisplay(state);
  Serial.printf("Turning display %s after %d seconds.\n", state ? "on" : "off", (millis() - lastMotion) / 1000);
  lastMotion = millis();
}
