#define N_OPENAQSTATIONS 1
#define N_OPENAQHISTORY 31
short pm25s[N_OPENAQSTATIONS][N_OPENAQHISTORY];
unsigned long syncTimes[N_OPENAQSTATIONS];

unsigned long sync;

void getOpenAQ() {
  // only update every 10 minutes
  if (sync > 0 && millis() - sync < 600000) {
    Serial.println("Not syncing");
    return;
  }
  sync = millis();
  String allData = getString("192.168.1.100", 8000, "/pm25.txt");
  uint16_t offset = 0;
  for (byte i = 0; i < N_OPENAQSTATIONS; i++) {
    uint16_t eol = allData.indexOf('\n', offset);
    String line = allData.substring(offset, eol);
//    x = getJSON("https://api.openaq.org/v2/measurements?date_from=2021-02-06&parameter=pm25&location_id=" + OPENAQSTATIONS[i] + "&order_by=datetime&sort=desc&limit=" + N_OPENAQHISTORY, false, &filter);
//    JsonArray measurements = x["results"].as<JsonArray>();
//    byte nmeasurements = measurements.size();
//    byte nMissing = 0;
//    byte firstHour = getHour(measurements[0]["date"]["local"].as<String>());
    byte mOffset = 0;
    for (byte j = 0; j < N_OPENAQHISTORY; j++) {
      byte eom = allData.indexOf(',', mOffset);
      pm25s[i][j] = line.substring(mOffset, eom).toInt();
      mOffset = eom+1;
      // from newest to oldest
//      byte hour = getHour(measurements[j]["date"]["local"].as<String>());
//      if ((hour + j + nMissing) % 24 == firstHour) {
//        pm25s[i][j + nMissing] = measurements[j]["value"].as<int>();
//      } else {
//        Serial.println("Some hour is missing!");
//        pm25s[i][j + nMissing] = -1;
//        nMissing++;
//      }
    }
    offset = eol+1;
  }
  Serial.println("Got AQ data");
}

unsigned long lastNtpTime = 0;
unsigned long lastNtpTimeMillis = RTCmillis();
unsigned long lastDataMillis = 0;

bool getNewData() {
  if (connectWifi("CMCC-iqVH", "k4vpan6q")) {
    // TODO move NTP to the back because it can be done async
    unsigned long ntpTime = getNTPTime();
    if (ntpTime != 0) {
      lastNtpTime = ntpTime;
      lastNtpTimeMillis = RTCmillis();
    }
    getOpenAQ();
    return true;
  } else {
    WiFi.printDiag(Serial);
    return false;
  }
}
