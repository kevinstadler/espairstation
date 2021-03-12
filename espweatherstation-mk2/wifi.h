#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
WiFiUDP ntpUDP;
NTPClient ntp(ntpUDP);

boolean connectWifi() {
  if (WiFi.status() == WL_CONNECTED) {
    return true;
  }
  Serial.print("Connecting to Wifi...");
  WiFi.reconnect();
  // TODO if status is 6 should wait LONGER! (while here, not if): https://github.com/esp8266/Arduino/issues/119#issuecomment-421530346
  if (WiFi.waitForConnectResult(20000) == WL_CONNECTED) {
    Serial.println("connected.");
    return true;
  } else {
    Serial.printf("failed! Wifi Status: %d, disconnecting.\n", WiFi.status());
    WiFi.disconnect();
    return false;
  }
}

unsigned long now;

bool updateCurrentTime() {
  Serial.print("Getting network time...");
  ntp.begin();
  // update takes a couple of attempts sometimes...
  if (!ntp.update()) {
    if (!ntp.forceUpdate()) {
      Serial.println("failed!"); // stays 0 if it fails on first attempt after boot...
      return false;
    }
  }
  ntp.end();
  now = ntp.getEpochTime();
  Serial.printf("success, now is: %d.\n", now);
  return true;
}

bool hasOneHourExpired(unsigned long since) {
  if (since >= now) { // >= so that if first NTP retrieval fails (all times are 0) all data is collected anyway
    // NTP seriously out of date...
    Serial.println("NTP seems seriously out of date, setting to value obtained from JSON...");
    now = since;
    return true;
  }
  Serial.printf("Checking timestamp: data is %d minutes old.\n", (now - since) / 60);
  return since + 60 * 60 < now;
}
