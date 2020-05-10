//// WIFI ////
#include <ESP8266WiFi.h>
#define AP_SSID "CU_DQsd"
#define AP_PASSWORD "xh6ctzf3"

// Jindingshan, Biji square, Longquan, Dongfeng square
#define N_AQISTATIONS 4
const String AQISTATIONS[] = { "@1382", "@1383", "@1380", "@1381" };
#define AQICNTOKEN "7ef04af09a2a93a67569a5c774a03c617e99103e"

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
#define MIIOTOKEN "789aa908daf3e6942ab4592d8eebd88d"

#define MOTION_PIN D3
volatile unsigned long lastMotion;

bool shouldDisplayBeOn() {
  return digitalRead(MOTION_PIN) == HIGH;
}

void ICACHE_RAM_ATTR motionDetectorInterrupt() {
  if (shouldDisplayBeOn()) {
    display.powerUp();
    // TODO draw everything here?
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

  display.begin(UCG_FONT_MODE_TRANSPARENT);
  display.clearScreen();
  display.setFontPosCenter();
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

#define INVALID_DATA -3.4028235E+38
const TempAndHumidity INVALID_PAIR = { INVALID_DATA, INVALID_DATA };
TempAndHumidity data[NumberOfSites] = { INVALID_PAIR, INVALID_PAIR, INVALID_PAIR };

void getData() {
  for (byte i = 0; i < NumberOfSites; i++) {
    getData(i);
  }
//  getWeather();
}

// see miio.ino
void getDeviceData();
// see aqi.ino
void getAQI();
// trivial
void getLocalSensor() {
  data[HERE] = dht.getTempAndHumidity();
  // TODO if (dht.getStatus() != ERROR_NONE) ...
}

void getData(byte location) {
  switch(location) {
    case OUT: getAQI(); break;
    case NEAR: getDeviceData(); break;
    case HERE: getLocalSensor(); break;
  }
//  if (shouldDisplayBeOn()) {
    draw(location);
//  }
}

#define ONEMINUTE 60000
#define POLLEVERY 5

void loop() {
  for (byte i = 0; i < POLLEVERY; i++) {
    getData(HERE);
    delay(ONEMINUTE);
  }
  Serial.println("-------------------------");
  connectWifi();
  getData();
}
