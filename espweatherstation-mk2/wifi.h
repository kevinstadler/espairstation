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
  if (WiFi.waitForConnectResult(10000) == WL_CONNECTED) {
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
      Serial.println("failed!");
      return false;
    }
  }
  ntp.end();
  now = ntp.getEpochTime();
  Serial.printf("success, now is: %d.\n", now);
  return true;
}

bool hasOneHourExpired(unsigned long since) {
  if (since >= now) {
    // NTP seriously out of date...
    Serial.println("NTP seems seriously out of date, setting to value obtained from JSON...");
    now = since;
    return true;
  }
  Serial.printf("Checking timestamp: data is %d minutes old.\n", now - since);
  return since + 60 * 60 < now;
}
