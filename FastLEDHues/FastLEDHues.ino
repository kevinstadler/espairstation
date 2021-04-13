#include <Adafruit_GFX.h>
#include <FastLED.h>
#include <FastLED_NeoMatrix.h>

#define PIN D7
#define mw 32
#define mh 8
#define NUMMATRIX (mw*mh)

CRGB matrixleds[NUMMATRIX];

FastLED_NeoMatrix *matrix = new FastLED_NeoMatrix(matrixleds, mw, mh, 1, 1, 
  NEO_MATRIX_BOTTOM     + NEO_MATRIX_RIGHT +
    NEO_MATRIX_COLUMNS + NEO_MATRIX_ZIGZAG );

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(0);

  FastLED.addLeds<NEOPIXEL,PIN>(matrixleds, NUMMATRIX); 
  matrix->begin();
  matrix->setTextWrap(false);
  matrix->fillScreen(0);

  CHSV hsv(10, 255, 50);
  CRGB rgb;
  hsv2rgb_rainbow(hsv, rgb);
  Serial.print(rgb[0]);
  Serial.print(rgb[1]);
  Serial.println(rgb[2]);
  hsv[0] = 0;
  hsv2rgb_rainbow(hsv, rgb);
  Serial.print(rgb[0]);
  Serial.print(rgb[1]);
  Serial.println(rgb[2]);
  hsv[0] = 245;
  hsv2rgb_rainbow(hsv, rgb);
  Serial.print(rgb[0]);
  Serial.print(rgb[1]);
  Serial.println(rgb[2]);

  if (false) {
    byte brightness = 90;
    for (byte i = 0; i < 32; i++) {
      byte hue = 24 - 2*i;
      Serial.println(hue);
//      matrix->setPassThruColor(CHSV(hue, 255, brightness));
      switch (i % 3) {
        case 0: // big contrast between all these steps
          matrix->setPassThruColor(CRGB(9, 1, 0));
          break;
        case 1:
          matrix->setPassThruColor(CRGB(10, 0, 0));
          break;
        case 2:
          matrix->setPassThruColor(CRGB(9, 0, 1));
          break;
      }
      matrix->drawLine(i, 0, i, 7, 0);
    }
    matrix->show();
  }
}

byte brightness = 55;
byte startHue = 96;
byte hueStep = 16;

void loop() {
  Serial.print(startHue);
  Serial.print(" - x*");
  Serial.print(hueStep);
  Serial.print(" @ ");
  Serial.println(brightness);

  // at 55 brightness, 80-89 are both full red and then 3 identical pink colors
  // at 60-105, there is only 1 red (at 80-84) then it becomes pink real quick
  // at 120, back to 1 red then 3 pink...
  // -> given the unpredictable abrupt changes when trying to be too fine-graied with hues,
  // better make it coarse and fix one hue for adjacent upper-half and lower-half width-5 intervals

  // unfortunate stepping artefacts but best one is first here
  // red is 55-65:
  // 0 = 90 + 16 per cat @ 50: 2 greens, 2 yellows, 2 oranges, red, pink, purple
  // 0 = 84 + 14 per cat @ 50: 1.5 greens, 3.5 yellows, dark orange, red, pink, purple
  // red in 45-55:
  // 0 = 84 + 18 per cat @ 50 brightness has 2 greens, 2 yellows, (dark) orange, red, pink, purple
  // 0 = 74 + 16 per cat @ 50 brightness has 1 green, 2 yellows, 2 oranges, red, pink, purple
  for (short pm = 0; pm < 160; pm += 5) {
    byte barHeight = (pm + 9) / 10;
    byte hueCat = (pm + 4) / 10;
    
    // 96 = green, 64 = yellow, 32 = orange, 0 = red, 224 = pink, 192 = purple, 160 = blue, 128 = aqua
    // 0           20           40           60       80          100           120         140ug
    // 10ug = 16degrees: left-shift 4 bits then divide by 10?
    // TODO starting at 96 gets yellow too quickly, start at 105+
    byte hue = startHue - hueStep * hueCat;
    // 100, 16
    // 105, 16
    Serial.print(pm);
    Serial.print(": ");
    Serial.println(hue);
    // https://github.com/marcmerlin/Framebuffer_GFX#color-management-and-adafruitgfx
    matrix->setPassThruColor(CHSV(hue, 255, brightness));
    matrix->drawLine(pm/5, 8, pm/5, 8 - barHeight, 0);
  }
//    matrix->setBrightness(20);
  matrix->setPassThruColor();
  matrix->setTextColor(matrix->Color(0, 0, 0));
  matrix->setCursor(0, 0);
//    matrix->print(F("test"));

  matrix->show();

  while (!Serial.available()) {
    delay(1000);
  }
  brightness = Serial.parseInt();
  startHue = Serial.parseInt();
  hueStep = Serial.parseInt();
}
