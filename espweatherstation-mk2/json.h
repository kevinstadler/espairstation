#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>

// openweathermap is up to 25k
DynamicJsonDocument doc(26000);

JsonVariant getJSON(String url, bool reuse = false) {
  doc.clear();
  HTTPClient http;
  http.setTimeout(10000);
  http.begin(url);//, "EEAA586D4F1F42F4185B7FB0F20A4CDD97477D99");
  int get = http.GET();
  if (get != 200) {
    Serial.printf("ERROR, http response was %d.\n", get);
  } else {
    DeserializationError e = deserializeJson(doc, http.getStream());
    if (e != DeserializationError::Ok) {
      Serial.printf("Failed to parse JSON: %s\n", e.c_str());
      Serial.println(doc.memoryUsage());
      Serial.println(doc.as<String>());
    }
  }
  if (!reuse) {
    http.end();
  }
  return doc.as<JsonVariant>();
}
