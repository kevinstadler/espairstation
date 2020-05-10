// icons: https://openweathermap.org/weather-conditions#How-to-get-icon-URL

// display.ucg_DrawBitmapLine(x, y, 0, const unsigned char*)

// api.openweathermap.org/data/2.5/onecall?lat=25.06&lon=102.7&units=metric&appid=267e24b06fd92cbeb4822210c8a3b5f1
void getWeather() {
  HTTPClient http2;
  http2.begin("http://api.openweathermap.org/data/2.5/onecall?lat=25.06&lon=102.7&units=metric&appid=267e24b06fd92cbeb4822210c8a3b5f1");
  if (http2.GET() != 200) {
    Serial.printf("ERROR, http response was %d.\n", http2.GET());
    return;
  }
  // https://openweathermap.org/api/one-call-api
  DynamicJsonDocument doc(http2.getSize()); // pretty big response...
  DeserializationError e = deserializeJson(doc, http2.getStream());
  if (e != DeserializationError::Ok) {
    Serial.println(e.c_str());
  }
  http2.end();

  JsonObject data = doc.as<JsonObject>();
  Serial.println("Current weather:");
  data = data["current"];
  Serial.println(data["temp"].as<String>());
  Serial.println(data["feels_like"].as<String>());
  Serial.println(data["humidity"].as<String>());
  Serial.println(data["dew_point"].as<String>());
  
//  if (response["status"].as<String>() != "ok") {
//  }
}
