// display pm25 concentrations mapped onto full-saturation full-value (spectrum-based) hues on a single RGB LED
// (this is before switching to FastLED with its rainbow-adjusted hue angles)

uint8_t hueToX(int16_t hue) {
  // assume C = 1
  // deviation is +- 60, which does not use 6 bits fully (but close enough)
  return 255 - (abs(hue % 120 - 60) << 2);
}

uint8_t hueToRGB(int16_t hue, uint8_t channel) {
  switch(channel) {
    case 0:
      return (hue < 60 || hue >= 300) ? 255 : (( hue >= 120 && hue < 240 ) ? 0 : hueToX(hue) );
    case 1:
      return hue >= 240 ? 0 : (( hue >= 60 && hue < 180 ) ? 255 : hueToX(hue) );
    case 2:
      return hue < 120 ? 0 : (( hue >= 180 && hue < 300 ) ? 255 : hueToX(hue) );
  }
}

// https://en.wikipedia.org/wiki/Hue#24_hues_of_HSL/HSV
uint8_t pm25ToRGB(short pm25, uint8_t channel) {
  // 10ug = 15 degrees of hue
  short cutoff = 200;
  if (channel == 0) {
    Serial.print((465 - (3 * min(pm25, cutoff)) / 2) % 360);
  }
  return hueToRGB((465 - (3 * min(pm25, cutoff)) / 2) % 360, channel);
}

void setup() {
  Serial.begin(115200);
  Serial.println();
  analogWriteRange(255);
  for (byte pm = 0; pm < 255; pm += 5) {
    Serial.print(pm);
    Serial.print("ug -> ");
    uint8_t rgb[3] = { pm25ToRGB(pm, 0),  pm25ToRGB(pm, 1),  pm25ToRGB(pm, 2) };
    Serial.printf(" (%i,%i,%i)\n", rgb[0], rgb[1], rgb[2]);
    // minus because PWM outputs the GROUND
    analogWrite(D8, 255-rgb[0]);//R
    analogWrite(D7, 255-rgb[1]);//G
    analogWrite(D6, 255-rgb[2]);//B
    delay(1000);
  }
}

void loop() {
}
