#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>

// openweathermap is up to 25k // as of August >30k!?
DynamicJsonDocument doc(40000);

JsonVariant getJSON(String url, bool reuse = false) {
  doc.clear();
  HTTPClient http;
  http.setTimeout(20000); // ooff GFW slows things recently...
  http.begin("192.168.1.100", 3128, url); // this actually works!!! (to go through proxy)
//  http.begin(url);//, "EEAA586D4F1F42F4185B7FB0F20A4CDD97477D99");
  int get = http.GET();
  if (get != 200) { // -1 = connection refused, -11 = timeout
    Serial.printf("ERROR, http response was %d.\n", get);
  } else {
    DeserializationError e = deserializeJson(doc, http.getStream());
    if (e != DeserializationError::Ok) {
      Serial.printf("Failed to parse JSON: %s\n", e.c_str());
      Serial.println(doc.memoryUsage());
      Serial.println(doc.as<String>().length());
//      Serial.println(doc.as<String>()); // this can raise errors of input was include etc
    }
  }
  if (!reuse) {
    http.end();
  }
  return doc.as<JsonVariant>();
}
