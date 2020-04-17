//// WIFI ////
#include <ESP8266WiFi.h>
#define AP_SSID "FILL IN HERE"
#define AP_PASSWORD "FILL IN HERE"

// Longquan, Dongfeng square, Jindingshan, Biji square
#define N_AQISTATIONS 4
const String AQISTATIONS[] = { "@1380", "@1381", "@1382", "@1383" };
#define AQICNTOKEN "FILL IN HERE"

#include <SPI.h>
#include "Ucglib.h"
#define RST_PIN  D6 // not necessary?
#define DC_PIN   D8 // necessary
#define CS_PIN   D10 // necessary
// _FT_HWSPI works the same?
Ucglib_SSD1351_18x128x128_HWSPI display(DC_PIN, CS_PIN, RST_PIN);

#include <DHTesp.h>
#define DHT_PIN D2
DHTesp dht;

#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#define MIIOTOKEN "FILL IN HERE"

#define MOTION_PIN D3
volatile unsigned long lastMotion;

void ICACHE_RAM_ATTR motionDetectorInterrupt() {
  if (digitalRead(MOTION_PIN) == HIGH) {
    display.powerUp();
  } else {
    display.powerDown();
    Serial.printf("Turning display off after %d seconds.\n", (millis() - lastMotion) / 1000);
  }
  lastMotion = millis();
}

void connectWifi() {
  if (WiFi.status() == WL_CONNECTED) {
    return;
  }
  WiFi.mode(WIFI_STA);
  WiFi.begin(AP_SSID, AP_PASSWORD);
  Serial.print("Connecting to Wifi...");
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.print("failed, retrying...");
    delay(1000);
  }
  Serial.println("connected.");
}

void setup() {
  digitalWrite(LED_BUILTIN, LOW);
  Serial.begin(115200);
  Serial.println("");

  display.begin(UCG_FONT_MODE_SOLID);
  display.clearScreen();
//  display.setFont(ucg_font_ncenR14_hr);
//  display.setFont(ucg_font_fub20_hf);
  // free universal regular/bold
  // transp/mono/height/8x8 + full/reduced/numbers
  // RED, YELLOW, GREEN, CYAN, BLUE, MAGENTA, BLACK, WHITE
  const byte barWidth = display.getWidth() / 8;
  const byte barHeight = display.getHeight();
  display.setColor(255, 0, 0);
  display.drawBox(0, 0, barWidth, barHeight);
  display.setColor(255, 255, 0);
  display.drawBox(barWidth, 0, barWidth, barHeight);
  display.setColor(0, 255, 0);
  display.drawBox(2 * barWidth, 0, barWidth, barHeight);
  display.setColor(0, 255, 255);
  display.drawBox(3 * barWidth, 0, barWidth, barHeight);
  display.setColor(0, 0, 255);
  display.drawBox(4 * barWidth, 0, barWidth, barHeight);
  display.setColor(255, 0, 255);
  display.drawBox(5 * barWidth, 0, barWidth, barHeight);
  display.setColor(0, 0, 0);
  display.drawBox(6 * barWidth, 0, barWidth, barHeight);
  display.setColor(255, 255, 255);
  display.drawBox(7 * barWidth, 0, barWidth, barHeight);

//  display.setFont(ucg_font_helvR24_tf); // helvetica
//  display.setFont(ucg_font_fur20_hf);
  display.setFont(ucg_font_fur17_hf);
  // alternatives: inconsolate ucg_font_inr*_
  display.setFontPosCenter();

  lastMotion = millis();
  dht.setup(DHT_PIN, DHTesp::DHT22);

  connectWifi();
  display.clearScreen();
  display.setColor(0, 0, 0);
  display.drawBox(0, 0, display.getWidth(), display.getHeight());
  getData();

  // keep screen on for at least 10 seconds before allowing screen off interrupt
  delay(10000);
  Serial.println("Attaching motion detector interrupt to powerUp/powerDown display.");
  if (digitalRead(MOTION_PIN) == LOW) {
    display.powerDown();
  }
  attachInterrupt(digitalPinToInterrupt(MOTION_PIN), motionDetectorInterrupt, CHANGE);
}

enum Location {
  OUT, HERE, NEAR,
  NumberOfSites
};

TempAndHumidity data[NumberOfSites];

void render() {
  for (byte i = 0; i < NumberOfSites; i++) {
    render(i);
  }
}

char formatted[5];
char* formatFloat(float value, byte minimumWidth, byte decimals) {
  // stringLength < 6
  return dtostrf(value, minimumWidth, decimals, formatted);
}
char* formatFloat(float value, byte decimals) {
  return formatFloat(value, 0, decimals);
}
char* formatFloat(float value) {
  return formatFloat(value, 0, 0);
}

void render(byte location) {
//  display.setColor(0, 0, 40, 80);
//  display.setColor(1, 80, 0, 40);
//  display.setColor(2, 255, 0, 255);
//  display.setColor(3, 0, 255, 255);
//  display.drawGradientBox(0, 0, display.getWidth(), display.getHeight());

  display.setPrintDir(0);

  byte y = 16 + 32 * location;

  display.setColor(100, 100, 110);
  if (location == HERE) {
    display.drawString(0, y, 0, "h:");
  } else if (location == NEAR) {
    display.drawString(0, y, 0, "x:");
  } else {
    display.drawString(0, y, 0, "o:");
  }
  byte x = 30;

  setTemperatureColor(data[location].temperature);
//  display.setColor(255, 168, 0);
  x += display.drawString(x, y, 0, formatFloat(data[location].temperature, 1, 0));
  display.drawGlyph(x, y, 0, 0xb0); // degree sign

  x = 72;
  display.setPrintPos(x, y);
  setComfortColor(data[location].temperature, data[location].humidity);
  display.print((byte) data[location].humidity);
  display.print("%");
}

void getData() {
  for (byte i = 0; i < NumberOfSites; i++) {
    getData(i);
  }
}

// see miio.ino
void getDeviceData();
// see aqi.ino
void getAQI();
// trivial
void getLocalSensor() {
  data[HERE] = dht.getTempAndHumidity();
}

void getData(byte location) {
  switch(location) {
    case OUT: getAQI(); break;
    case NEAR: getDeviceData(); break;
    case HERE: getLocalSensor(); break;
  }
  render(location);
}

void loop() {
  // poll every 5 minutes = 300 seconds
//  delay(300000);
  delay(300000);
  Serial.println("-------------------------");
  connectWifi();
  getData();
}
