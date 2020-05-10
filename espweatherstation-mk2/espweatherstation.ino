#define ONEMINUTE 60000
#define LOOPDELAY 5*ONEMINUTE // do something every 5 minutes

#define AP_SSID "CU_DQsd"
#define AP_PASSWORD "xh6ctzf3"
#include "wifi.h"
#include "math.h"

void drawAQI();
void drawWeather();
void drawLocalData();

#define LOCALSENSOR_PIN D1
#include "localsensor.h"

const String OPENWEATHERMAP_KEY = "267e24b06fd92cbeb4822210c8a3b5f1";
#include "json.h"
#include "weather.h"

// Jindingshan, Biji square, Longquan, Dongfeng square
#define N_AQISTATIONS 4
const String AQISTATIONS[] = { "@1382", "@1383", "@1380", "@1381" };
#define AQICNTOKEN "7ef04af09a2a93a67569a5c774a03c617e99103e"
#define N_AQIHISTORY 15
#include "aqi.h"

#define MIIO_TOKEN "789aa908daf3e6942ab4592d8eebd88d"
#define DESIRED_DEWPOINT 13.0
#include "miio.h"

// display/SPI: steer clear of GPIO 0, 2 and 15 (D3, D4, D8)
#define RST_PIN D6
#define DC_PIN  D0
#define CS_PIN  D8 // should be this
#include "display.h"

#define MOTIONSENSOR_PIN D3
#include "motionsensor.h"

void setup() {
  Serial.begin(115200);
  Serial.println("\nSetting up...");

  display.begin();
  display.fillScreen(BLACK);
  display.setTextWrap(false);

//  drawIcon("01d", 8, 8);
//  drawIcon("01n", 8, 24);
//  drawIcon("02d", 24, 8);
//  drawIcon("02n", 24, 24);
//  drawIcon("03d", 40, 8);
//  drawIcon("04d", 56, 8);
//  drawIcon("09d", 72, 8);
//  drawIcon("10d", 88, 8);
//  drawIcon("10n", 88, 24);
//  drawIcon("11d", 40, 24);
//  drawIcon("13d", 56, 24);
//  drawIcon("50d", 72, 24);
//  delay(100000);

  localSensor.setup(LOCALSENSOR_PIN, DHTesp::DHT22);
  delay(1000);
  getLocalSensor();

  WiFi.mode(WIFI_STA);
  WiFi.begin(AP_SSID, AP_PASSWORD);
  if (connectWifi()) {
    getWeather();
    getAQI();
    getHumidifierData();
  }

  Serial.print("Attaching motion detector interrupt...");
  attachInterrupt(digitalPinToInterrupt(MOTIONSENSOR_PIN), motionDetectorInterrupt, CHANGE);
  Serial.println("setup finished.");
  // keep screen on for at least 10 seconds before allowing screen off interrupt
  delay(10000);
  motionDetectorInterrupt();
  delay(LOOPDELAY);
}

void loop() {
  if (connectWifi()) {
    if (!updateCurrentTime()) {
      Serial.println("Manually adding time to the system clock...");
      now += LOOPDELAY;
    }
    getWeather();
  
    getHumidifierData();
    getLocalSensor();
  
    getAQI();
    delay(LOOPDELAY);
  } else {
    Serial.println("Not connected to wifi, waiting...");
    now += ONEMINUTE;
    delay(ONEMINUTE);
  }
}
