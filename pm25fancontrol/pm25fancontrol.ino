#define AP_SSID "QT"
#define AP_PASSWORD "quokkathedog"

#define PLUGTOKEN "5fa7fb65c6995fc5a2d8766f035f96db"
#define HUMIDIFIERTOKEN "71574ff36422e7a4ca3971d88aba3a3d"

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

// from /common/
#include <miio.h>
VerboseBlockingMiioDevice *fanPlug;
#include "humidifier.h"
Humidifier *humidifier;

#include <SoftwareSerial.h>
SoftwareSerial soft(D7, D5); // RX TX
#include "PMS.h"
PMS pms(soft);
PMS::DATA data;

#include "colors.h"
#define R_PIN D1
#define G_PIN D8
#define B_PIN D2

//  analogWriteRange(1023);
void setLED(byte col[]) {
  analogWrite(R_PIN, col[0]);
  analogWrite(G_PIN, col[1]);
  analogWrite(B_PIN, col[2]);
}

byte cols[2][3];
void setColor(byte value, byte index) {
  float catPos = getStepwiseLinearCatPos(value, AQI);
  byte cat = (byte) catPos;
  float mixing = catPos - cat;
  for (byte i = 0; i < 3; i++) {
    int diff = AQICOLORS[cat+1][i] - AQICOLORS[cat][i];
    Serial.println(diff);
    cols[index][i] = AQICOLORS[cat][i] + mixing * diff;
  };
  Serial.println(value);
  Serial.println(catPos);
  Serial.println(cols[index][0]);
  Serial.println(cols[index][1]);
  Serial.println(cols[index][2]);
}

// log: outdoor-pm is implicit as first, add: insidepm, plugstate (0/1), temp (NA if none), humidity (NA if none), targethumidity (NA if none)
byte log(String csvs) {
  HTTPClient http;
  http.begin("192.168.1.100", 8000, "/cgi-bin/write.py?" + csvs);
  int r = http.GET();
  Serial.println("Logged.");
  // retrieves outdoor level
  long last = http.getStream().parseInt();
  http.end();
  return last;
}

#define MINUTE 60e6
void goToSleep(long ms = 15*MINUTE) {
  setLED(BLACK);
  WiFi.disconnect();
  ESP.deepSleep(ms);
}

// blinking starts and ends on cols[0]
void blink(byte col2[]) {
  for (byte i = 0; i < 9; i++) {
    setLED(i % 2 ? col2 : cols[0]);
    delay(800);
    setLED(BLACK);
    delay(10);
  }
}

void blink() {
  blink(cols[1]);
}

volatile bool plugState;
volatile short temperature;
volatile short humidity = -1;
byte newHumidity;
String getAirString() {
  return humidity == -1 ? "NA,NA,NA" : String(temperature) + "," + String(humidity) + "," + String(newHumidity);
}

void setup() {
  setLED(BLACK);
  Serial.begin(115200);
  Serial.println("\nLast reset because: " + ESP.getResetReason());
  soft.begin(9600);
//  wifi_fpm_set_sleep_type(LIGHT_SLEEP_T);
  soft.flush();
  pms.wakeUp();
  pms.passiveMode();

  Serial.print("WiFi...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(AP_SSID, AP_PASSWORD);
  if (WiFi.waitForConnectResult(20000) == WL_CONNECTED) {
    Serial.println("connected.");
  } else {
    Serial.printf("failed! Wifi Status: %d, disconnecting.\n", WiFi.status());
    goToSleep();
  }
  fanPlug = new VerboseBlockingMiioDevice(new IPAddress(192, 168, 2, 101), PLUGTOKEN, &Serial);
  humidifier = new Humidifier(new IPAddress(192, 168, 2, 100), HUMIDIFIERTOKEN, &Serial);
  if (fanPlug->awaitConnection()) {
    fanPlug->get("power", [plugState](JsonVariant result) {
      plugState = result.as<String>().equals("on");
      Serial.println(plugState);
      Serial.println("Result: " + result.as<String>());
    });
    // {'id': 1, 'method': 'get_prop', 'params': ['power', 'temperature']}
    // {'id': 1, 'method': 'set_power', 'params': ['on']} // or 'off'
  }
  if (!ESP.getResetReason().equals("Software/System restart")) {
    if (humidifier->awaitConnection()) {
      setupIfNecessary(humidifier);
      if (humidifier->get("Humidity_Value", [humidity](JsonVariant result) {
        humidity = result.as<int>();
      }) && humidifier->get("TemperatureValue", [temperature](JsonVariant result) {
        temperature = result.as<int>();
      })) {
        newHumidity = humidifier->setHumidityForDewPoint(temperature, 12.0);
      }
    }
  }
  byte n = 5;
  byte got = 0;
  byte errors = 0;
  uint16_t average = 0;
  while (got < n) {
    if (errors >= n) {
      // try a reset (if we haven't already tried that
      if (ESP.getResetReason().equals("Software/System restart")) {
        // ok just give up...
        // TODO log to website
        goToSleep();
      } else {
        Serial.println("Too many errors, trying to see if a RESET helps");
        log("NA,NA," + getAirString());
        ESP.reset();
      }
    }
    pms.requestRead();
    if (pms.readUntil(data, 2000) && data.PM_AE_UG_2_5 > 0) {
      uint16_t dp = data.PM_AE_UG_2_5;
      Serial.println(dp);
      if (got > 0 && abs(dp - average/got) >= 5) {
        Serial.println("0\n0\n0\n0"); // new value seems off, culling old measurements (back to 1)
        average = dp;
        got = 1;
      } else {
        average += dp;
        got++;
      }
    } else {
      Serial.println("some read error");
      errors++;
      // clear buffer
      while (soft.available()) {
        soft.read();
      }
    }
  }
  pms.sleep();
  
  average = average / n;
  Serial.println(average);
  bool newPlugState = average >= 15;

  if (fanPlug->isConnected() && newPlugState != plugState) {
    Serial.println("Changing plugstate");
    fanPlug->sendCommand("set_power", "\"" + (newPlugState ? String("on") : String("off")) + "\"", [](JsonVariant result) {
      // TODO actually do something with this?
      Serial.println("Result: " + result.as<String>());
    });
  }
  byte outside = log(String(average) + "," + (newPlugState ? "4" : "0") + "," + getAirString());
  int outsideDiff = outside - average;

  if (outsideDiff <= -10) {
    // outside is better, that's fuucked
    setColor(average, 0);
    blink(WHITE);
//    newPlugState = false;
  } else {
    // outside is worse, turn on / blink between the two
    setColor(average, 0);
    setColor(outside, 1);
    blink();
  }
  // default sleep is 15m
  goToSleep();

//  wifi_fpm_open();
//  wifi_fpm_do_sleep(10E6);
//  delay(10e3 + 1); // delay needs to be 1 mS longer than sleep or it only goes into Modem Sleep
}

void loop() {
  Serial.println("LOOP - BUT WHY");
}
