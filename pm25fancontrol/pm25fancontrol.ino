#define AP_SSID "QT"
#define AP_PASSWORD "quokkathedog"

#define PLUGTOKEN "5fa7fb65c6995fc5a2d8766f035f96db"
#define HUMIDIFIERTOKEN "71574ff36422e7a4ca3971d88aba3a3d"

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

// from /common/
#include <miio.h>
VerboseBlockingMiioDevice *fanPlug;
#include "humidifier.h"
Humidifier *humidifier;

#include <SoftwareSerial.h>
SoftwareSerial soft(D7, D5); // RX TX
#include "PMS.h"
PMS pms(soft);
PMS::DATA data;
#define NREADINGS 5 // average over 5 consecutive readings

uint16_t readPM25() {
  pms.requestRead();
  if (pms.readUntil(data, 2000)) {
    return data.PM_AE_UG_2_5;
  } else {
    // clear buffer
    while (soft.available()) {
      soft.read();
    }
    return 0;
  }
}


#include "colors.h"
#define R_PIN D1
#define G_PIN D8
#define B_PIN D2

//  analogWriteRange(1023);
void setLED(byte col[]) {
  analogWrite(R_PIN, col[0]);
  analogWrite(G_PIN, col[1]);
  analogWrite(B_PIN, col[2]);
}

byte cols[2][3];
void setColor(byte value, byte index) {
  float catPos = getStepwiseLinearCatPos(value, AQI);
  byte cat = (byte) catPos;
  float mixing = catPos - cat;
  for (byte i = 0; i < 3; i++) {
    int diff = AQICOLORS[cat+1][i] - AQICOLORS[cat][i];
    Serial.println(diff);
    // not even sure how much the analogWriteRange is but just shift down 1 for dimming...
    cols[index][i] = byte(AQICOLORS[cat][i] + mixing * diff) >> 1;
  };
  Serial.println(value);
  Serial.println(catPos);
  Serial.println(cols[index][0]);
  Serial.println(cols[index][1]);
  Serial.println(cols[index][2]);
}

HTTPClient http;
byte outdoorPM() {
  http.setReuse(true);
  http.begin("192.168.1.100", 8000, "/pm25.txt");
  int r = http.GET();
  Serial.println(http.getStream().readStringUntil('\n')); // skip timestamp
  long pm = 0;
  while (http.getStream().available()) {
    pm = http.getStream().parseInt();
    if (pm >= 0) {
      break;
    }
    // skip rest of line
    http.getStream().readStringUntil('\n');
  }
  http.end();
  return pm;
}

#include <rtc_memory.h>
RtcMemory rtcMemory;
typedef struct {
  byte lastAverage;
} MyData;

// log: outdoor-pm is implicit as first, add: insidepm, plugstate (0/1), temp (NA if none), humidity (NA if none), targethumidity (NA if none)
void log(String csvs) {
  http.begin("192.168.1.100", 8000, "/cgi-bin/write.py?" + csvs);
  http.GET();
  http.end();
}

#define MINUTE 60e6
long sleepTime = 15*MINUTE;
void goToSleep(long ms = sleepTime) {
  setLED(BLACK);
  WiFi.disconnect();
  Serial.print("Going to sleep for ");
  Serial.print(ms / MINUTE);
  Serial.println("m");
  ESP.deepSleep(ms);
}

// blinking starts and ends on cols[0]
void blink(byte col2[]) {
  for (byte i = 0; i < 9; i++) {
    setLED(i % 2 ? col2 : cols[0]);
    delay(800);
    setLED(BLACK);
    delay(10);
  }
}

void blink() {
  blink(cols[1]);
}

volatile bool plugState;
volatile int16_t temperature;
volatile int16_t humidity = -1;
byte newHumidity;

void setup() {
  setLED(BLACK);
  Serial.begin(115200);
  Serial.println("\nLast reset because: " + ESP.getResetReason());
  soft.begin(9600);
  soft.flush();
  pms.wakeUp();
  pms.passiveMode();

  Serial.print("WiFi...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(AP_SSID, AP_PASSWORD);

  byte got = 0;
  byte errors = 0;
  uint16_t dp; // last local read
  uint16_t average = 0; // cumulative average
  while (got < NREADINGS) {
    if (errors >= NREADINGS) {
      // try a reset (if we haven't already tried that
      if (ESP.getResetReason().equals("Software/System restart")) {
        // ok just give up...
        average = 0;
        break;
      } else {
        Serial.println("Too many errors, trying to see if a RESET helps");
        ESP.reset();
      }
    }
    dp = readPM25();
    if (dp == 0) {
      Serial.println("some read error");
      errors++;
    } else {
      Serial.println(dp);
      if (got > 0 && abs(dp - average/got) >= 5) {
        Serial.println("--"); // new value seems off, culling old measurements (back to 1)
        average = dp;
        got = 1;
      } else {
        average += dp;
        got++;
      }
    }
  }
  pms.sleep();
  average = average / NREADINGS;
  Serial.println(average);

  if (WiFi.waitForConnectResult(20000) == WL_CONNECTED) {
    Serial.println("connected.");
  } else {
    Serial.printf("failed! Wifi Status: %d, disconnecting.\n", WiFi.status());
    goToSleep();
  }

  fanPlug = new VerboseBlockingMiioDevice(new IPAddress(192, 168, 2, 101), PLUGTOKEN, &Serial);
  humidifier = new Humidifier(new IPAddress(192, 168, 2, 100), HUMIDIFIERTOKEN, &Serial);
  if (fanPlug->awaitConnection()) {
    fanPlug->get("power", [plugState](JsonVariant result) {
      plugState = result.as<String>().equals("on");
      Serial.println("Plug state: " + result.as<String>());
      Serial.println(plugState);
    });
    // {'id': 1, 'method': 'get_prop', 'params': ['power', 'temperature']}
    // {'id': 1, 'method': 'set_power', 'params': ['on']} // or 'off'
  } else {
    Serial.println("couldn't connect to plug.");
    // double but cap at 3h
    sleepTime = min(2*sleepTime, long(180*MINUTE));
  }
//  if (!ESP.getResetReason().equals("Software/System restart")) {
  if (humidifier->awaitConnection()) {
    setupIfNecessary(humidifier);
    if (humidifier->get("Humidity_Value", [humidity](JsonVariant result) {
      humidity = result.as<int>();
    }) && humidifier->get("TemperatureValue", [temperature](JsonVariant result) {
      temperature = result.as<int>();
    })) {
      newHumidity = humidifier->setHumidityForDewPoint(temperature, 12.0);
    }
  } else {
    Serial.println("couldn't connect to humidifier.");
  }

  bool newPlugState;
  if (average > 0 && fanPlug->isConnected()) {
    // TODO this assumes we got a plugState result returned as well, not necessarily true

    // hysteresis-based on/off
    newPlugState = plugState;
    if ((!plugState && average >= 17) || (plugState && average <= 12)) {
      newPlugState = !newPlugState;
    }

    byte outside = outdoorPM();
    short outsideDiff = outside - average;

    if (outsideDiff <= -10) {
      // outside is better, that's fuucked
      setColor(average, 0);
      blink(WHITE);
      newPlugState = false;
    } else {
      // outside is worse, turn on / blink between the two
      setColor(average, 0);
      setColor(outside, 1);
      blink();
    }
  }

  rtcMemory.begin(); // ignore return
  MyData* data = rtcMemory.getData<MyData>();
  if (data->lastAverage == 0) {
    Serial.println("First time boot, no previous info");
  } else {
    Serial.print("Last was ");
    Serial.print(data->lastAverage);
    Serial.print(", now is ");
    Serial.println(average);
    // if the plug has been on, it was set on by previous loop but the average has gone up, give it a break
    if (plugState && average > data->lastAverage && !ESP.getResetReason().equals("External System")) {
      newPlugState = false;
      sleepTime = 60*MINUTE;
      Serial.println("Doesn't seem to be working, maybe a door is open or something.");
    }
  }
  // saving data
  data->lastAverage = average;
  rtcMemory.save();

  if (fanPlug->isConnected() && newPlugState != plugState) {
    Serial.println("Changing plugstate");
    fanPlug->sendCommand("set_power", "\"" + (newPlugState ? String("on") : String("off")) + "\"", [](JsonVariant result) {
      // TODO actually do something with this? -- adjust plugStateString depending on success!
      Serial.println("Result: " + result.as<String>());
    });
  }

  // log
  String plugStateString = fanPlug->isConnected() ? (newPlugState ? "4" : "0") : "NA";
  String humidityString = humidity == -1 ? "NA,NA,NA" : String(temperature) + "," + String(humidity) + "," + String(newHumidity);
  log(String(average) + "," + plugStateString + "," + humidityString);

  goToSleep();
}

void loop() {
  Serial.println("LOOP - BUT WHY");
  goToSleep();
}
