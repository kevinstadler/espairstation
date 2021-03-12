#include <ESP8266WiFi.h>
#define AP_SSID "QT"
#define AP_PASSWORD "quokkathedog"

#include <ESPMiio.h>
#define PLUGTOKEN "5fa7fb65c6995fc5a2d8766f035f96db"
#define MIIOTIMEOUT 1000 // ms
MiioDevice *device;

#include <SoftwareSerial.h>
SoftwareSerial soft(D5, D7); // RX TX
#include "PMS.h"
PMS pms(soft);
PMS::DATA data;

#include <ESP8266HTTPClient.h>

const String MIIO_ERRORSTRING[5] = { "not connected", "timeout", "busy", "invalid response", "message creation failed" };
// 0 == waiting, 1 == OK, 2 == error
volatile byte responseStatus = 1;
bool sendCommand(String command, String jsonArgs, void (*callback)(JsonVariant result), void (*errorCallback)(byte errorCode)) {
//  if (!connectDevice()) {
//    Serial.println("ERROR: did not send command because no device connected.");
//    return false;
//  }
  Serial.print("Sending " + command + " " + jsonArgs + "...");
  byte waits = 0;
  while (device->isBusy()) {
    if (waits >= 20) {
      Serial.println("ERROR: failed to send command because UDP device is busy");
      return false;
    }
    Serial.print("waiting...");
    delay(200);
    waits++;
  }
  responseStatus = 0;
  if (device->send(command.c_str(), jsonArgs.c_str(),
      [command, jsonArgs, callback](MiioResponse response) {
        responseStatus = 1;
        Serial.println("response: " + response.getResult().as<String>() + ".");
        if (callback != NULL) {
          callback(response.getResult());
        }
      }, [errorCallback](byte error) {
        responseStatus = 2;
        Serial.println("error: " + MIIO_ERRORSTRING[error]);
        if (errorCallback != NULL) {
          errorCallback(error);
        }
      })) {
      Serial.print("sent, awaiting response..");
      while (responseStatus == 0) {
        Serial.print(".");
        delay(200);
      }
      delay(200); // one more for good measure...
      return responseStatus != 2;
    } else {
      Serial.println("failed.");
      return false;
    }
}

int log(uint16_t average, String webString) {
  HTTPClient http;
  http.begin("192.168.1.100", 8000, "/cgi-bin/write.py?" + String(average) + "," + webString);
  int r = http.GET();
  http.end();
  return r;
}

#define MINUTE 60e6
volatile bool plugState;  
void setup() {
  Serial.begin(115200);
  Serial.println("\nLast reset because: " + ESP.getResetReason());
  soft.begin(9600);
//  wifi_fpm_set_sleep_type(LIGHT_SLEEP_T);
  soft.flush();
  pms.wakeUp();
  pms.passiveMode();

  WiFi.mode(WIFI_STA);
  WiFi.begin(AP_SSID, AP_PASSWORD);
  if (WiFi.waitForConnectResult(20000) == WL_CONNECTED) {
    Serial.println("connected.");
  } else {
    Serial.printf("failed! Wifi Status: %d, disconnecting.\n", WiFi.status());
    WiFi.disconnect();
    ESP.deepSleep(10*MINUTE);
  }
  device = new MiioDevice(new IPAddress(192, 168, 2, 101), PLUGTOKEN, MIIOTIMEOUT);
  responseStatus = 0;
  device->connect([](MiioError e) {
    responseStatus = 2;
    Serial.printf("failed (error %d)...", e);
  });
  // await connection
  while (!device->isConnected() && responseStatus == 0) {
    Serial.print(".");
    delay(200);
  }
  if (device->isConnected()) {
    Serial.println("connected.");
      sendCommand("get_prop", "\"power\"", [plugState](JsonVariant result) {
        plugState = result.as<String>().equals("on");
        Serial.println(plugState);
          Serial.println("Result: " + result.as<String>());
        }, NULL);
    // {'id': 1, 'method': 'get_prop', 'params': ['power', 'temperature']}
    // {'id': 1, 'method': 'set_power', 'params': ['on']} // or 'off'
  } else {
    Serial.println("Failed to connect to plug.");
    ESP.deepSleep(10*MINUTE);
  }
  byte n = 5;
  byte got = 0;
  byte errors = 0;
  uint16_t average = 0;
  while (got < n) {
    if (errors >= n) {
      Serial.println(log(0, plugState ? "on" : "off"));
      ESP.deepSleep(10*MINUTE);
    }
    pms.requestRead();
    if (pms.readUntil(data, 2000) && data.PM_AE_UG_2_5 > 0) {
      uint16_t dp = data.PM_AE_UG_2_5;
      Serial.println(dp);
      if (got == 0 || abs(dp - average/got) <= 5) {
        average += dp;
        got++;
        Serial.println(got);
      } else {
        Serial.println("new value seems off, culling old measurements (back to 1)");
        average = dp;
        got = 1;
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
  String stateString = newPlugState ? "on" : "off";
  String webString = newPlugState ? "4" : "0";

  Serial.println(log(average, webString));
  if (newPlugState != plugState) {
    Serial.println("Changing plugstate");
    sendCommand("set_power", "\"" + stateString + "\"", [](JsonVariant result) {
        Serial.println("Result: " + result.as<String>());
        ESP.deepSleep(10*MINUTE); // WAKE_RF_DEFAULT
      }, NULL);
  } else {
    Serial.println("Sleep");
    ESP.deepSleep(10*MINUTE);
  }
//  wifi_fpm_open();
//  wifi_fpm_do_sleep(10E6);
//  delay(10e3 + 1); // delay needs to be 1 mS longer than sleep or it only goes into Modem Sleep
}

void loop() {
  Serial.println("LOOP - BUT WHY");
}
