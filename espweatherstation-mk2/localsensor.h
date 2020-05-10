#include <DHTesp.h>
DHTesp localSensor;

AirData localData = INVALID_AIRDATA;

void getLocalSensor() {
  TempAndHumidity dhtData = localSensor.getTempAndHumidity();
  if (localSensor.getStatus() == DHTesp::ERROR_NONE) {
    localData = { dhtData.temperature, dhtData.humidity, calculateDewPoint(dhtData.temperature, dhtData.humidity) };
  } else {
    Serial.printf("Error reading local sensor: %s\n", localSensor.getStatusString());
    localData = INVALID_AIRDATA;
  }

  drawLocalData();
}
