#include <WiFiUdp.h>
#include <NTPClient.h>
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);
HTTPClient http;

#define N_AQIHISTORY 9
float pm25s[N_AQISTATIONS][N_AQIHISTORY];
byte latestData[N_AQISTATIONS];
time_t syncTimes[N_AQISTATIONS];

boolean isExpired(byte station, time_t now) {
  if (now < 100 || syncTimes[station] == 0) {
    Serial.println("NTP and/or sync time unavailable, assume station data has expired.");
    return true;
  } else {
    Serial.printf("Station #%d expires in %d minutes\n", station, 60 - (now - syncTimes[station]) / 60);
  }
  // expect updates approximately every hour...
  return syncTimes[station] + 60 * 60 < now;
}

// -CATOFFSET, good, moderate, sensitive, unhealthy, very, hazardous
// numbers given are for the UPPER boundary of the categories
// categories actually go from [value[i] - value[0], value[i+1]]
// yes really there are strange absurd non-linearities at the category boundaries.....
// https://www.airnow.gov/sites/default/files/2018-05/aqi-technical-assistance-document-may2016.pdf
const float AQI[] = { -1, 50, 100, 150, 200, 300, 500 };
const float PM25[] = { -0.1, 12.0, 35.4, 55.4, 150.4, 250.4, 500.4 };
const float CAQIPM25[] = { 0, 15, 30, 55, 110, 500 };

byte getStepwiseLinearCat(float value, const float *thresholds) {
  byte cat = 0;
  while (value > thresholds[cat+1]) {
    cat++;
  }
  return cat;
}

float mean(float v1, float v2) {
  return (v1 + v2) / 2;
}

float getStepwiseLinearCatPos(float value, const float *thresholds) {
  byte cat = getStepwiseLinearCat(value, thresholds);
  // category mean = half-way between lower and lower bound
  float catMean = mean(thresholds[cat] - thresholds[0], thresholds[cat + 1] - thresholds[0]);
    // add/subtract some (up to 0.5) but don't go below 0
  return max(0.0f, cat + (value - catMean) / (thresholds[cat + 1] - thresholds[cat]));
  // are we in the higher or the lower half of the current category?
//  if (value >= catMean) {
  // alternative strategy: don't do +- 0.5 around category mean, but straight linear interpolation
  // between each pair of means
//  float higherMean = mean(thresholds[higherCat] - thresholds[0], thresholds[higherCat + 1]);
//  float lowerMean = mean(thresholds[higherCat - 1] - thresholds[0], thresholds[higherCat]);
//  float progressIntoHigherCategory = (thresholds[higherCat] - value) / (thresholds[higherCat] - thresholds[higherCat - 1]);
}

float aqiToPM25(int aqi) {
  byte aqiCat = getStepwiseLinearCat(aqi, AQI);
  // compute correct offset
  // so this should be mapping e.g. [51, 100] onto [12.1, 35.4]...
  const byte lowerAqiBoundary = AQI[aqiCat] - AQI[0];
  float percentageOffset = (aqi - lowerAqiBoundary) / (AQI[aqiCat+1] - lowerAqiBoundary);
  float lowerPM25Boundary = PM25[aqiCat] - PM25[0];
  return lowerPM25Boundary + percentageOffset * (PM25[aqiCat+1] - lowerPM25Boundary);
}

void invalidateStation(byte station) {
  pm25s[station][latestData[station]] = -1;
}

void getAQI(byte station, DynamicJsonDocument &doc) {
  Serial.print("Querying AQI station " + AQISTATIONS[station] + "...");
  http.begin("http://api.waqi.info/feed/" + AQISTATIONS[station] + "/?token=" + AQICNTOKEN);
  if (http.GET() != 200) {
    Serial.printf("ERROR, http response was %d.\n", http.GET());
    invalidateStation(station);
    return;
  }
  // .getStream() would be better for memory, but causes a reboot...
  deserializeJson(doc, http.getString());
  JsonObject response = doc.as<JsonObject>();
  if (response["status"].as<String>() != "ok") {
    Serial.println("ERROR, JSON response status is: " + response["status"].as<String>());
    invalidateStation(station);
    return;
  }
  response = response["data"];
  struct tm timestamp;
  const char *syncTime = response["debug"]["sync"].as<const char*>();
  if (strptime(syncTime, "%Y-%m-%dT%H:%M:%S+%z", &timestamp)) {
    Serial.print("response ok but error parsing parsing debug sync time...");
    // TODO unset or set to now or something....
  } else {
    time_t tm = mktime(&timestamp);
    // mktime doesn't take time zones into account, so do this beauty....
    tm -= 60*60 * atoi(syncTime + 20);
    if (tm == syncTimes[station]) {
      Serial.println("response ok but no new data.");
      return;
    }
    syncTimes[station] = tm;
  }
//        String dp = response["dominentpol"].as<String>();
  latestData[station] = (latestData[station] + 1) % N_AQIHISTORY;
  response = response["iaqi"];
  pm25s[station][latestData[station]] = aqiToPM25(response["pm25"]["v"].as<int>());
  Serial.printf("%d AQI is %.1fug/m3 PM2.5...", response["pm25"]["v"].as<int>(), pm25s[station][latestData[station]]);
  // weather data is city-wide, so just take latest from whichever station
  data[OUT].temperature = response["t"]["v"].as<int>();
  data[OUT].humidity = response["h"]["v"].as<int>();
  Serial.println("done.");
}

void getAQI() {
  Serial.print("Getting network time...");
  timeClient.begin();
  // update takes a couple of attempts sometimes...
  if (!timeClient.update()) {
    if (!timeClient.forceUpdate()) {
      Serial.print("failed! Just adding 5 minutes of system clock...");
    }
  }
  timeClient.end();
  time_t now = timeClient.getEpochTime();
  Serial.printf("now is %d.\n", now);

  byte station = 0;
  while (true) {
    if (isExpired(station, now)) {
      break;
    } else if (++station == N_AQISTATIONS) {
      return;
    }
  }

  // if we're here there's at least one expired station...
  DynamicJsonDocument doc(2000);
  while (station < N_AQISTATIONS) {
    if (isExpired(station, now)) {
      getAQI(station, doc);
    }
    station++;
  }
  http.end();
  // add five minutes
  now += 300;
//  Serial.printf("Next poll will be at around :%s\n", (timeClient.getMinutes() + 5) % 60);
}
