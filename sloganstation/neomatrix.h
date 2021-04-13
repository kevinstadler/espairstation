#include <Adafruit_GFX.h>
#include <Adafruit_NeoMatrix.h>
#include <Adafruit_NeoPixel.h>
#include <AniMatrix.h>
#include <thresholds.h>

Adafruit_NeoMatrix matrix = Adafruit_NeoMatrix(32, 8, NEOMATRIX_PIN, NEO_MATRIX_BOTTOM + NEO_MATRIX_RIGHT + NEO_MATRIX_COLUMNS + NEO_MATRIX_ZIGZAG);

const uint32_t kevin = matrix.Color(0, 0, 255);
const uint32_t ellen = matrix.Color(255, 0, 200); // 50 isn't visible on brightness 10

const bool kevinWalking[][3] = { { false, false, false }, // M
                                 { false, false, false }, // T
                                 { false, true, true },   // W
                                 { false, true, true },   // T
                                 { true, true, true },    // F
                                 { true, true, true },    // S
                                 { true, false, false } };// S

byte dayOfWeek(unsigned long t) {
  return ((t / 86400) + 3) % 7; // Mon = 0, Sun = 6
}

uint32_t pm25colors[20];

void initMatrix(byte brightness = 10) {
  matrix.begin();
  matrix.setBrightness(brightness);
  matrix.clear();
  matrix.drawPixel(0, 0, matrix.Color(255, 0, 0));
  matrix.show();
  for (byte i = 0; i < 20; i++) {
    pm25colors[i] = matrix.Color(pm25ToRGB(5 + i*10, 0), pm25ToRGB(5 + i*10, 1), pm25ToRGB(5 + i*10, 2));
  }
}

void drawDoggie(unsigned long t) {
  byte d = dayOfWeek(t);
  Serial.println(d);
  for (byte i = 0; i < 3; i++) {
    matrix.drawPixel(i, 0, kevinWalking[d][i] ? kevin : ellen);
  }
}

void drawPM25() {
  Serial.println("Drawing PM25");
  short mx = 1; // make sure magnitude is at least 1
  for (byte i = 0; i < N_OPENAQSTATIONS; i++) {
    for (byte j = 0; j < N_OPENAQHISTORY; j++) {
      if (pm25s[i][j] > mx) {
        mx = pm25s[i][j];
      }
    }
  }
//  int magnitude = ceil(float(mx) / 79.0); // magnitude in multiples of 80ug
  for (byte i = 0; i < 1; i++) {
    for (byte j = 0; j < N_OPENAQHISTORY; j++) {
      if (pm25s[i][j] <= 0) {
        continue;
      }
      //uint16_t col = getColor(pm25s[i][j], AQI, AQI_COLORS);
      uint16_t col = mixColors(pm25s[i][j], AQI, AQI_COLORS);
      // 10ug per height, and force 0ug off the screen (y=8): [1, 12], [13, 24], ...
      matrix.drawLine(N_OPENAQHISTORY - j, 8, N_OPENAQHISTORY - j, 7 - (pm25s[i][j]-1) / 10, col);
    }
  }
//  if (pm25 >= 0) {
//    uint16_t col = mixColors(pm25, AQI, AQI_COLORS);
//    if (pm25 > pm25s[0][0]) {
//      col = RED;
//    } else {
//      col = BLACK;
//    }
//    matrix.drawPixel(N_OPENAQHISTORY, 7 - pm25 / (10*magnitude), col);
//  }
}
