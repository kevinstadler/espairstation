#include <WiFiUdp.h>
#include <NTPClient.h>
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

float pm25s[N_AQISTATIONS];
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
  // are we in the higher or the lower half of the current category...
//  byte higherCat = cat + (value >= mean(thresholds[cat] - thresholds[0], thresholds[cat + 1]));
  // take the two category means..
//  float higherMean = mean(thresholds[higherCat] - thresholds[0], thresholds[higherCat + 1]);
//  float lowerMean = mean(thresholds[higherCat - 1] - thresholds[0], thresholds[higherCat]);
  return cat + 1 - (thresholds[cat + 1] - value) / (thresholds[cat + 1] - thresholds[cat]);
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
  pm25s[station] = -1;
}

void getAQI(byte station, HTTPClient &http, DynamicJsonDocument &doc) {
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
  response = response["iaqi"];
  pm25s[station] = aqiToPM25(response["pm25"]["v"].as<int>());
  Serial.printf("%d AQI is %.1fug/m3 PM2.5...", response["pm25"]["v"].as<int>(), pm25s[station]);
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

  HTTPClient http;
  DynamicJsonDocument doc(2000);
  while (station < N_AQISTATIONS) {
    if (isExpired(station, now)) {
      getAQI(station, http, doc);
    }
    station++;
  }
  http.end();
  renderAQI();
  // add five minutes
  now += 300;
//  Serial.printf("Next poll will be at around :%s\n", (timeClient.getMinutes() + 5) % 60);
}

void renderAQI() {
  // average over all available station data
  byte usedStations = 0;
  float pm25 = 0;
  for (byte i = 0; i < N_AQISTATIONS; i++) {
    if (pm25s[i] >= 0) {
      pm25 += pm25s[i];
      usedStations++;
    }
  }
  display.setColor(0, 0, 0);
  display.drawBox(0, 128 - 16, display.getWidth(), 16);
  if (usedStations == 0) {
    return;
  }
  pm25 = pm25 / usedStations;
  Serial.printf("PM25 average over %d stations is %.1fug/m3.\n", usedStations, pm25);

  // interpolate color between the two adjacent aqi categories
  setAqiColor(getStepwiseLinearCatPos(pm25, PM25));
  // until AQI 150 (=PM25 55) one aqi index is < 1ug PM25, at 150 it switches to ~2mg per 1 aqi
  // so until 100ug we can render a formatted float, after we can just round to int
  byte decimals = pm25 < 100 ? 1 : 0;
  byte x = 8;
  x += display.drawString(0, 128 - 16, 0, formatFloat(pm25, 2, decimals));
  display.drawString(x, 128 - 16, 0, "ug/m3");
  // greek mu is 0x6d in ucg_font_symb12/14/18/24_tr
}
