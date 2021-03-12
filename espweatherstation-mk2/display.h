#include "thresholds.h"

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1331.h>
#include <SPI.h>
Adafruit_SSD1331 display = Adafruit_SSD1331(&SPI, CS_PIN, DC_PIN, RST_PIN);

//#include <Fonts/FreeSans9pt7b.h>
//#define DEFAULT_FONT &FreeSans9pt7b
#define FONT_OFFSET 0

#define AQIBOXSIZE 4
#define AQISECTIONHEIGHT AQIBOXSIZE * N_AQISTATIONS
#define AQIBOXAREAWIDTH AQIBOXSIZE * N_AQIHISTORY
#define AQIOFFSET display.height() - AQISECTIONHEIGHT
// display is 96x64

void printTemperature(float temperature) {
  if (isValid(temperature)) {
    uint16_t color = getColor(temperature, TEMPERATURE, TEMPERATURE_COLORS);
    display.setTextColor(color, BLACK);
    display.printf("%2.0f", temperature);
    display.drawCircle(display.getCursorX() + 1, display.getCursorY() + 1, 1, color);
  } else {
    display.setTextColor(WHITE, BLACK);
    display.print("-- "); // extra space to cover up the degree sign/circle
  }
}

bool drawDewPointMarker(float dewPoint, String label, byte y, bool highlightBackground) {
  if (isValid(dewPoint)) {
    // there is a 2 offset at the beginning but we'll leave that off so that
    // characters are printed with the correct offset (centered at their value)
    display.setCursor(0 + max(0, int(3*dewPoint)), y);
    display.setTextColor(getColor(dewPoint, DEWPOINT, DEWPOINT_COLORS), highlightBackground ? RED : BLACK);
    display.print(label);
  }
  return isValid(dewPoint);
}

bool drawDewPointMarker(float dewPoint, String label, byte y) {
  return drawDewPointMarker(dewPoint, label, y, false);
}

void drawLocalData() {
  display.fillRect(0, 19, 96, 24, BLACK);

  for (byte i = 0; i < 6; i++) {
    // 3 pixels per degree, 5 degree thresholds differences
    display.fillRect(2 + 15*i, 28, 15, 6, DEWPOINT_COLORS[i]);
  }
  // humidifier goal marker
  display.fillRect(byte(2 + 3*DESIRED_DEWPOINT), 26, 1, 10, getColor(DESIRED_DEWPOINT, DEWPOINT, DEWPOINT_COLORS));
  // draw what's the acceptable range for 35-55% humidity?
//  calculateDewPoint(humidifierData.temperature, 55);

  if (drawDewPointMarker(outdoorData.dewPoint, "O", 19)) {
//    display.printf("%.0f%%", outdoorData.humidity);
  }
  if (drawDewPointMarker(localData.dewPoint, "H", 36)) {
//    display.printf("%.0f%%", localData.humidity);
  }
  if (drawDewPointMarker(humidifierData.dewPoint, "X", 36, tankEmpty)) {
//    display.printf("%.0f%%", humidifierData.humidity);
  }
}

void drawCloud(byte x, byte y, uint16_t color) {
  // clockwise from bottom left
  display.fillCircle(x - 3, y + 2, 3, color);
  display.fillCircle(x - 2, y - 1, 3, color);
  display.fillCircle(x + 3, y + 1, 2, color);
  display.fillCircle(x + 4, y + 3, 2, color);
  display.fillRect(x - 3, y + 2, 8, 4, color);
}

void drawRain(byte x, byte y) {
  display.drawLine(x - 2, y + 2, x - 2, y + 3, RAINDROPS);
  display.drawLine(x - 3, y + 5, x - 4, y + 7, RAINDROPS);

  display.drawLine(x + 1, y + 3, x   , y + 5, RAINDROPS);

  display.drawLine(x + 4, y + 2, x + 4, y + 3, RAINDROPS);
  display.drawLine(x + 3, y + 5, x + 2, y + 7, RAINDROPS);
}

// draw 15x15 icon (central pixel is at x, y)
void drawIcon(String iconStr, byte x, byte y) {
  // clear icon
  display.fillRect(x - 7, y - 7, 15, 15, BLACK);
  if (iconStr.length() == 0) {
    Serial.println("Invalid icon string: \"" + iconStr + "\".");
    return;
  }
  // see weather.h
  byte icon = iconStr.toInt();
//  Serial.printf("Drawing icon %d\n", icon);
  uint16_t color = iconStr.endsWith("d") ? ORANGE : MOON;
  switch(icon) {
    case CLEAR:
      display.fillCircle(x, y, 6, color);
      break;
    case FEWCLOUDS: // few clouds: sun first, cloud over it
      display.fillCircle(x + 2, y - 1, 4, color);
      drawCloud(x, y, LIGHTCLOUD);
      break;
    case SCATTEREDCLOUDS:
      drawCloud(x, y, LIGHTCLOUD);
      break;
    case OVERCAST: // broken clouds
      drawCloud(x + 1, y - 1, DARKCLOUD);
      drawCloud(x - 1, y + 1, LIGHTCLOUD);
      break;
    case SHOWERS:
      drawCloud(x + 1, y - 2, DARKCLOUD);
      drawCloud(x - 1, y, LIGHTCLOUD);
      drawRain(x - 1, y);
      break;
    case RAIN:
      display.fillCircle(x + 2, y - 2, 4, color);
      drawCloud(x, y - 1, LIGHTCLOUD);
      drawRain(x, y);
      break;
    case THUNDERSTORM:
      drawCloud(x + 1, y - 2, DARKCLOUD);
      drawCloud(x - 1, y, LIGHTCLOUD);
      display.drawLine(x - 3, y + 2, x - 3, y + 3, ORANGE);
      display.drawFastHLine(x - 3, y + 3, 2, ORANGE);
      display.drawLine(x - 1, y + 4, x - 2, y + 7, ORANGE);
      break;
    case SNOW:
      display.drawFastHLine(x - 5, y, 11, WHITE);
      display.drawFastVLine(x, y - 5, 11, WHITE);
      display.drawLine(x - 4, y - 4, x + 4, y + 4, WHITE);
      display.drawLine(x - 4, y + 4, x + 4, y - 4, WHITE);
      display.drawPixel(x, y, BLACK);
      display.drawFastHLine(x - 1, y - 4, 3, WHITE);
      display.drawFastHLine(x - 1, y + 4, 3, WHITE);
      display.drawFastVLine(x - 4, y - 1, 3, WHITE);
      display.drawFastVLine(x + 4, y - 1, 3, WHITE);
      break;
    case MIST:
      display.drawFastHLine(x - 2, y - 4, 6, LIGHTCLOUD);
      display.drawFastHLine(x - 5, y - 2, 9, LIGHTCLOUD);
      display.drawFastHLine(x - 3, y    , 9, LIGHTCLOUD);
      display.drawFastHLine(x - 5, y + 2, 10, LIGHTCLOUD);
//      display.drawFastHLine(x - 3, y + 4, 8, LIGHTCLOUD);
      display.drawFastHLine(x - 3, y + 4, 6, LIGHTCLOUD);
      break;
    default:
      Serial.println("Unknown icon status!?");
      break;
  }
}

void drawWeather() {
  display.setTextSize(1);
  display.setCursor(16, 0);
  printTemperature(feelsLike);
  display.setCursor(16, 8);
  printTemperature(feelsLikeShade);

  display.setCursor(48, 0);
  printTemperature(forecastDay[0]);
  display.setCursor(48, 8);
  printTemperature(forecastNight[0]);

  display.setCursor(81, 0);
  printTemperature(forecastDay[1]);
  display.setCursor(81, 8);
  printTemperature(forecastNight[1]);

  drawIcon(currentIcon, 7, 7);
  drawIcon(forecastIcon[0], 39, 7);
  drawIcon(forecastIcon[1], 72, 7);
}

void drawAQI() {
  // average over all available station data
  byte usedStations = 0;
  float pm25 = 0;
  for (byte i = 0; i < N_AQISTATIONS; i++) {
    if (isValid(pm25s[i][latestData[i]])) {
      pm25 += pm25s[i][latestData[i]];
      usedStations++;
    }
  }

  if (usedStations == 0) {
    // TODO draw question mark?
    return;
  }
  pm25 = pm25 / usedStations;
  Serial.printf("PM25 average over %d stations is %.1fug.\n", usedStations, pm25);

  // interpolate color between the two adjacent aqi categories
  display.setTextColor(mixColors(pm25, CAQI, CAQI_COLORS), BLACK);
  display.setTextSize(1);
  display.setCursor(AQIBOXAREAWIDTH + 2, 52 + FONT_OFFSET);
  // until AQI 150 (=PM25 55) one aqi index is < 1ug PM25, at 150 it switches to ~2mg per 1 aqi
  // so until 100ug we can render a formatted float, after we can just round to int
  display.printf("%.0fug", pm25);
//  x += display.drawGlyph(x, y, 0, 0x6d);

  // draw boxes, top down
  for (int i = 0; i < N_AQISTATIONS; i++) {
    // draw right to left
    for (int j = 0; j < N_AQIHISTORY; j++) {
      byte h = ( latestData[i] - j + N_AQIHISTORY ) % N_AQIHISTORY;
      display.fillRect(AQIBOXAREAWIDTH - (j + 1) * AQIBOXSIZE, AQIOFFSET + i*AQIBOXSIZE, AQIBOXSIZE, AQIBOXSIZE,
        pm25s[i][h] > 0 ? mixColors(pm25s[i][h], CAQI, CAQI_COLORS) : BLACK);
    }
  }
}
