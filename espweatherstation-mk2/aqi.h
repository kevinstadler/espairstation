float pm25s[N_AQISTATIONS][N_AQIHISTORY];
byte latestData[N_AQISTATIONS];
unsigned long syncTimes[N_AQISTATIONS];

// numbers given are for the *inclusive* **upper** boundaries of the categories:
// (none) good, moderate, sensitive, unhealthy, very, hazardous
// categories actually go from [value[i] + 0.1, value[i+1]]
// yes really there are strange absurd non-linearities at the category boundaries.....
// https://www.airnow.gov/sites/default/files/2018-05/aqi-technical-assistance-document-may2016.pdf
const float AQI[]  = { 0,   50,  100,  150,   200,   300,   500 };
const float PM25[] = { 0.0, 12.0, 35.4, 55.4, 150.4, 250.4, 500.4 };

float aqiToPM25(int aqi) {
  byte aqiCat = getStepwiseLinearCat(aqi, AQI);
  // compute correct offset
  // so this should be mapping e.g. [51, 100] onto [12.1, 35.4]...
  const byte lowerAqiBoundary = AQI[aqiCat - 1] + 1;
  float percentageOffset = (aqi - lowerAqiBoundary) / (AQI[aqiCat] - lowerAqiBoundary);
  float lowerPM25Boundary = PM25[aqiCat - 1] + 0.1;
  return lowerPM25Boundary + percentageOffset * (PM25[aqiCat] - lowerPM25Boundary);
}

bool isExpired(byte station) {
  if (hasOneHourExpired(syncTimes[station])) {
    return true;
  }
  Serial.printf("Station #%d expires in %d minutes.\n", station, 60 - (now - syncTimes[station]) / 60);
  return false;
}

void getAQI(byte station) {
  Serial.print("Querying AQI station " + AQISTATIONS[station] + "...");
  JsonObject response = getJSON("http://api.waqi.info/feed/" + AQISTATIONS[station] + "/?token=" + AQICNTOKEN);
  if (response["status"].as<String>() != "ok") {
    Serial.println("ERROR, JSON response status is: " + response["status"].as<String>());
    pm25s[station][latestData[station]] = INVALID_DATA;
    return;
  }
  response = response["data"];
  struct tm timestamp;
  const char *syncTime = response["debug"]["sync"].as<const char*>();
  if (strptime(syncTime, "%Y-%m-%dT%H:%M:%S+%z", &timestamp)) {
    Serial.print("response ok but error parsing parsing debug sync time...");
    syncTimes[station] = now; // maybe not the best...
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
  Serial.println("done.");
}

void getAQI() {
  byte station = 0;
  while (true) {
    if (isExpired(station)) {
      break;
    } else if (++station == N_AQISTATIONS) {
      return;
    }
  }

  // if we're here there's at least one expired station...
  while (station < N_AQISTATIONS) {
    if (isExpired(station)) {
      getAQI(station);
    }
    station++;
  }
  drawAQI();
  // add five minutes
//  now += 300;
//  Serial.printf("Next poll will be at around :%s\n", (timeClient.getMinutes() + 5) % 60);
}
