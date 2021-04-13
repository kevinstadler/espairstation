volatile long lastTime;

void ICACHE_RAM_ATTR onTimeOut() {
  Serial.println("Screen timeout");
  Serial.println((millis() - lastTime) / 1000);
}

void ICACHE_RAM_ATTR motionDetectorInterrupt() {
  // 312500 ticks = 1s
  //15s = 15, 30 = 3s??
  timer1_write(26.5*312500); // longest timer at 3.2us/tick = 26843542.4us
  lastTime = millis();
  Serial.println("Interrupted");
}

//void setupMotionSensor() {
//  Serial.println("Attaching motion detector interrupt...");
//  timer1_attachInterrupt(onTimeOut);
//  timer1_enable(TIM_DIV256, TIM_EDGE, TIM_SINGLE);
//  attachInterrupt(digitalPinToInterrupt(MOTIONSENSOR_PIN), motionDetectorInterrupt, CHANGE);
//  motionDetectorInterrupt();
//}
