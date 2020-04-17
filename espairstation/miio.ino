#include <ESPMiio.h>
IPAddress device_ip(192, 168, 1, 4);
MiioDevice device(&device_ip, MIIOTOKEN, 1000);

void sendCommand(String command, String jsonArgs) {
  sendCommand(command, jsonArgs, NULL);
}

void sendCommand(String command, String jsonArgs, void (*callback)(JsonVariant result)) {
  if (!checkDevice()) {
    return; // fail
  }
  Serial.print("Sending " + command + " " + jsonArgs + "...");
  if (device.isBusy()) {
    Serial.println("waiting once...");
    delay(1000);
  }
  if (device.send(command.c_str(), jsonArgs.c_str(),
      [&](MiioResponse response) {
        Serial.println("Response to " + command + " " + jsonArgs + ": " + response.getResult().as<String>());
        if (callback != NULL) {
          callback(response.getResult());
        }
      },
      [&](MiioError e){
        Serial.printf("Command error: %d\n", e);
      })) {
      Serial.println("sent.");
  } else {
    Serial.println("failed.");
    // TODO invalidate data...
  }
  delay(500); // 300 is sometimes dodge...for setting values especially!?
}

volatile byte requiresSetup = false;

void connectDevice() {
  if (device.isBusy()) {
    delay(500);
    Serial.print("...");
  }
  device.connect([](MiioError e) {
    Serial.printf("Connection error: %s\n", e);
  });
  // await connection
  delay(1000);
}

//volatile bool awaitingResponse = false;

bool checkDevice() {
//  Serial.println(device == NULL);
  if (device.isConnected()) {
    if (requiresSetup) {
      Serial.println("Setting up device...");
      requiresSetup = false;
      sendCommand("SetTipSound_Status", "0");
      sendCommand("SetLedState", "0");
      sendCommand("Set_OnOff", "1");
      // Set_HumiValue and Set_HumidifierGears will be called by adjustHumidifer()...
    }
    return true;
  }
  Serial.print("Connecting to Miio device...");
//    MiioDevice dev(&device_ip, MIIOTOKEN, 1000);
//    device = &dev;
  connectDevice();

  if (device.isConnected()) {
    Serial.println("connected.");
  } else {
    // hmmmm...
    byte i = 2;
    while (i < 10) {
//      IPAddress device_ip(WiFi.localIP()[0], WiFi.localIP()[1], WiFi.localIP()[2], i);
//      device_ip[3] = i;
//      MiioDevice dev(&device_ip, MIIOTOKEN, 1000);
//      device = &dev;
      Serial.printf("Trying %d.%d.%d.%d...", device_ip[0], device_ip[1], device_ip[2], device_ip[3]);
      connectDevice();
      if (device.isConnected()) {
        Serial.println("connected!");
        break;
      } else {
        Serial.println("no luck.");
      }
      i++;
    }
    if (!device.isConnected()) {
      return false;
    }
  }

  sendCommand("get_prop", "\"Led_State\"", [](JsonVariant result) {
    if (result.as<int>() != 0) {
      Serial.println("Device looks uninitiated, queueing setup.");
      requiresSetup = true;
//      sendCommand("get_prop", "[\"OnOff_State\"]");
//      sendCommand("get_prop", "[\"Led_State\"]");
    }
  });
  // leave time for response to set requiresSetup to true...
  delay(1000);
  return true;
}

byte gearState;

void getDeviceData() {
  sendCommand("get_prop", "\"Humidity_Value\"", [](JsonVariant result) {
    data[NEAR].humidity = result.as<int>();
  });
  sendCommand("get_prop", "\"TemperatureValue\"", [](JsonVariant result) {
    data[NEAR].temperature = result.as<int>();
  });
  sendCommand("get_prop", "\"Humidifier_Gear\"", [](JsonVariant result) {
    gearState = result.as<byte>();
  });

  // this isn't nice, but what is...
  delay(1000);
  render(NEAR);
  adjustHumidifier();
  Serial.println("Disconnecting.");
  device.disconnect();
}

void adjustHumidifier() {
  // find highest comfortable humidity for temperature at the device
  // max comfortable is 60, max recommended 55, count down from there...

  // examples: VeryComfy range at 14deg is 77+, 18deg is 60-72, at 22deg 47-56, at 26deg 37-44,
  // at 30deg 29-35, at 34deg 24-28
  // start at 55 and go down until it's either very comfy (or dry)...
  int humidity = 55;
  while (dht.computePerception(data[NEAR].temperature, humidity, false) > Perception_VeryComfy) {
    humidity--;
  }
    // at 22 degress anything between 31 and 70% is acceptable, so might have to find better way...
//    dht.getComfortRatio(helper, data[NEAR].temperature, humidity, false);
//    if ((helper & Comfort_TooHumid) == 0) {
//      break;
//    }
  Serial.printf("New ideal humidity value should be: %d.\n", humidity);
  // calling Set_HumidifierGears (re)sets HumiValue to 54, so do this first
  if (gearState != 4) {
    sendCommand("Set_HumidifierGears", "4");
  }
  sendCommand("Set_HumiValue", String(humidity));
//  sendCommand("Set_OnOff", "1");
}
