//// WIFI ////
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#define AP_SSID "FILL IN HERE"
#define AP_PASSWORD "FILL IN HERE"

//// AQI ////
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
// Longquan, Dongfeng square, Jindingshan, Biji square
#define NSTATIONS 4
const String STATIONS[] = { "@1380", "@1381", "@1382", "@1383" };
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

#include <ESPMiio.h>
#define TOKEN "FILL IN HERE"
IPAddress device_ip(192, 168, 1, 5);
MiioDevice device(&device_ip, TOKEN, 2000);

#define MOTION_PIN D3
volatile unsigned long lastMotion;

void ICACHE_RAM_ATTR motionDetectorInterrupt() {
  if (digitalRead(MOTION_PIN) == HIGH) {
    display.powerUp();
    Serial.println("on");
  } else {
    display.powerDown();
    Serial.print("off after ");
    Serial.println((millis() - lastMotion) / 1000);
  }
  lastMotion = millis();
}

void connectWifi() {
  digitalWrite(LED_BUILTIN, LOW);
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
  Serial.begin(115200);
  Serial.println(LED_BUILTIN);
  Serial.println(D9);
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
  Serial.println("start");

  dht.setup(DHT_PIN, DHTesp::DHT22);

  connectWifi();
  display.clearScreen();
  display.setColor(0, 0, 0);
  display.drawBox(0, 0, display.getWidth(), display.getHeight());
  getData();

  // keep screen on for at least 10 seconds before allowing screen off interrupt
  delay(10000);
  Serial.println("Attaching motion detector powerup/down interrupt");
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
ComfortState comfort[NumberOfSites];
float pm25;

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

void render(byte location) {
//  display.setColor(0, 0, 40, 80);
//  display.setColor(1, 80, 0, 40);
//  display.setColor(2, 255, 0, 255);
//  display.setColor(3, 0, 255, 255);
//  display.drawGradientBox(0, 0, display.getWidth(), display.getHeight());

  display.setPrintDir(0);

  byte y = 16 + 32 * location;

  display.setColor(255, 168, 0);
  int x = display.drawString(0, y, 0, formatFloat(data[location].temperature, 1, 0));
//  display.drawCircle(x+2, y-10, 2, UCG_DRAW_ALL); // y minus half the font size?
  display.drawGlyph(x, y, 0, 0xb0);

  display.setColor(32, 64, 255);
  display.setPrintPos(64, y);
  display.print((byte) data[location].humidity);
  display.print("%");
}

void getData() {
  for (byte i = 0; i < NumberOfSites; i++) {
    getData(i);
  }
}

// helper
void getLocalSensor() {
  data[HERE] = dht.getTempAndHumidity();
}

void getData(byte location) {
  switch(location) {
  case OUT: getAQI(); break;
  case NEAR: getDeviceData(); break;
  case HERE: getLocalSensor(); break;
  }
  // Comfort_OK _TooHot _TooCold _TooDry _TooHumid _HotAndHumid etc
  dht.getComfortRatio(comfort[location], data[location].temperature, data[location].humidity, false);
  render(location);
}

void getAQI() {
  HTTPClient http;

  const size_t capacity = 2*JSON_ARRAY_SIZE(2) + 9*JSON_OBJECT_SIZE(1) + 3*JSON_OBJECT_SIZE(2) + 2*JSON_OBJECT_SIZE(3) + 2*JSON_OBJECT_SIZE(8) + 530;
  DynamicJsonDocument doc(capacity);

  byte stationsUsed = 0;
  for (byte i = 0; i < NSTATIONS; i++) {
    http.begin("http://api.waqi.info/feed/" + STATIONS[i] + "/?token=" + AQICNTOKEN);
    if (http.GET() == 200) {
      // .getStream() would be better for memory, but causes a reboot...
      deserializeJson(doc, http.getString());
      JsonObject response = doc.as<JsonObject>();
      if (response["status"].as<String>() == "ok") {
        stationsUsed++;
        response = response["data"];
//        String dp = response["dominentpol"].as<String>();
        response = response["iaqi"];
        data[OUT].temperature = response["t"]["v"].as<int>();
        data[OUT].humidity = response["h"]["v"].as<int>();
        pm25 = aqiToPM25(response["pm25"]["v"].as<int>());
        renderAQI();
      }
    }
  }
  http.end();
  // TODO do averaging?
}

// good, moderate, sensitive, unhealthy, very, hazardous
const float AQI[] = { 0, 50, 100, 150, 200, 300, 500 };
const float AQI_OFFSET = 1;
const float PM25[] = { 0.0, 12.0, 35.4, 55.4, 150.4, 250.4, 500.4 };
const float PM25_OFFSET = 0.1;

byte getAqiCat() {
  return getAqiCat(pm25, PM25);
}

byte getAqiCat(float pm25, const float *thresholds) {
  byte aqiCat = 0;
  while (pm25 > thresholds[aqiCat+1]) {
    aqiCat++;
  }
  return aqiCat;
}

float aqiToPM25(int aqi) {
  byte aqiCat = getAqiCat(aqi, AQI);
  float percentageOffset = (AQI[aqiCat+1] - aqi) / (AQI[aqiCat+1] - AQI[aqiCat]);
  // FIXME incorporate offset?
  return PM25[aqiCat] + percentageOffset * (PM25[aqiCat+1] - PM25[aqiCat]);
}

const byte AQI_R[] = { 0, 0xFF, 0xFF, 0xFF, 0x99, 0x7E };
const byte AQI_G[] = { 0xE4, 0xFF, 0x7E, 0, 0, 0xFF };
const byte AQI_B[] = { 0, 0, 0, 0, 0x4C, 0x23 };

void renderAQI() {
  byte aqiCat = getAqiCat();
  display.setColor(AQI_R[aqiCat], AQI_G[aqiCat], AQI_B[aqiCat]);
  // until AQI 150 (=PM25 55) one aqi index is < 1ug PM25, at 150 it switches to ~2mg per 1 aqi
  // so until 100ug we can render a formatted float, after we can just round to int
  byte decimals = pm25 < 100 ? 1 : 0;
  byte x = display.drawString(0, 128 - 16, 0, formatFloat(pm25, 2, decimals));
  display.drawString(x, 128 - 16, 0, "ug/m3");
  // greek mu is 0x6d in ucg_font_symb12/14/18/24_tr
}

void sendCommand(String command, String jsonArgs) {
  sendCommand(command, jsonArgs, NULL);
}

volatile bool waitingForResponse = false;

void sendCommand(String command, String jsonArgs, void (*callback)(JsonVariant result)) {
  if (!checkDevice()) {
    return; // fail
  }
  Serial.print("Sending " + command + " " + jsonArgs);
  while (device.isBusy() || waitingForResponse) {
    delay(50);
    Serial.print(".");
  }
  waitingForResponse = true;
  if (device.send(command.c_str(), jsonArgs.c_str(),
      [&command, &jsonArgs, &callback](MiioResponse response) {
        Serial.println("Response to " + command + " " + jsonArgs + ": " + response.getResult().as<String>());
        if (callback != NULL) {
          callback(response.getResult());
        }
        // free memory
        waitingForResponse = false;
      },
      [](MiioError e){
        Serial.printf("Command error: %d\n", e);
      })) {
      Serial.println(" sent.");
  } else {
    Serial.println(" failed.");
  }
  delay(400); // 400 buhao, 500 ok
}

bool checkDevice() {
  if (device.isConnected()) {
    return true;
  } else {
    Serial.print("Connecting to Miio device...");
    device.connect([](MiioError e) {
      Serial.println("error");
      return false;
    });
    delay(1000);
  }

  if (!device.isConnected()) {
    Serial.println("connected but then connection lost?");
    return false;
  }

  Serial.println("connection established, setting up...");
  sendCommand("get_prop", "\"TipSound_State\"", [](JsonVariant result) {
    if (result.as<int>() == 1) {
      Serial.println("TODO set sound off etc");
//      sendCommand("get_prop", "[\"OnOff_State\"]");
//      sendCommand("get_prop", "[\"Led_State\"]");
    }
  });
  sendCommand("SetTipSound_Status", "0");
  sendCommand("Set_OnOff", "1");
  sendCommand("SetLedState", "0");
  sendCommand("Set_HumidifierGears", "4");
  sendCommand("Set_HumiValue", "60");
  return true;
}

void getDeviceData() {
  sendCommand("get_prop", "\"Humidity_Value\"", [](JsonVariant result) {
    data[NEAR].humidity = result.as<int>();
//    render(NEAR);
  });
  sendCommand("get_prop", "\"TemperatureValue\"", [](JsonVariant result) {
    data[NEAR].temperature = result.as<int>();
    render(NEAR);
    adjustHumidifier();
  });
}

void adjustHumidifier() {
  // TODO compute comfort of NEAR sensor and call
//  sendCommand("Set_HumidifierGears", "\"4\"");
//  sendCommand("Set_HumiValue", "");
}

void loop() {
  getLocalSensor();
  render(HERE);
//  getAQI();
  render(OUT);
  delay(5000);
}
