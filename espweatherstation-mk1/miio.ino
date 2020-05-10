#include <ESPMiio.h>

#define TIMEOUT 500

// this is just the default IP, if no device is found there it will scan the local network
IPAddress *ip = new IPAddress(192, 168, 1, 4);
MiioDevice *device = new MiioDevice(ip, MIIOTOKEN, TIMEOUT);

void sendCommand(String command, String jsonArgs) {
  sendCommand(command, jsonArgs, NULL);
}

void sendCommand(String command, String jsonArgs, void (*callback)(JsonVariant result)) {
  sendCommand(command, jsonArgs, callback, NULL);
}

void sendCommand(String command, String jsonArgs, void (*callback)(JsonVariant result), void (*errorCallback)(byte errorCode)) {
  if (!checkDevice()) {
    return; // fail
  }
  Serial.print("Sending " + command + " " + jsonArgs + "...");
  for (byte waits = 0; waits < 10; waits++) {
    if (device->isBusy()) {
      Serial.print("waiting...");
      delay(100);
    } else {
      break;
    }
  }
  if (device->send(command.c_str(), jsonArgs.c_str(),
      [callback, command, jsonArgs](MiioResponse response) {
        Serial.println("Response to " + command + " " + jsonArgs + ": " + response.getResult().as<String>());
        if (callback != NULL) {
          callback(response.getResult());
        }
      }, errorCallback)) {
    Serial.println("sent.");
  } else {
    Serial.println("failed.");
    // TODO invalidate data...
  }
}

volatile byte requiresSetup = false;

void findDevice() {
  IPAddress local = WiFi.localIP();

  // scan through the local network IP range to discover a MiIo device
  for (byte i = 2; i < 10; i++) {
    if (i == local[3]) {
      continue; // don't try to connect to yourself
    }
    delete device;
    delete ip;
    ip = new IPAddress(local[0], local[1], local[2], i);
    Serial.print("Trying ");
    Serial.print(*ip);
    Serial.print("...");
    device = new MiioDevice(ip, MIIOTOKEN, TIMEOUT);

    device->connect();

    // leave AT LEAST 150ms for a reliable connection
    delay(TIMEOUT);
    if (device->isConnected()) {
      Serial.println("found!");
      break;
    } else {
      Serial.println("unavailable");
    }
  }
}

bool connectDevice() {
  if (device->isBusy()) {
    delay(500);
    Serial.print("...");
  }
  device->connect([](MiioError e) {
    Serial.println(e);
  });
  // await connection
  delay(1000);
  if (!device->isConnected()) {
    Serial.println("failed to connect, initiating scan.");
    findDevice();
  }
}

//volatile bool awaitingResponse = false;

bool checkDevice() {
//  Serial.println(device == NULL);
  if (device->isConnected()) {
    if (requiresSetup) {
      Serial.println("Setting up device...");
      requiresSetup = false;
      sendCommand("SetTipSound_Status", "0");
      sendCommand("SetLedState", "0");
      // this one always breaks....
      sendCommand("Set_OnOff", "1");
      // Set_HumiValue and Set_HumidifierGears will be called by adjustHumidifer()...
    }
    return true;
  }
  Serial.print("Connecting to Miio device...");
//    MiioDevice dev(&device_ip, MIIOTOKEN, 1000);
//    device = &dev;
  connectDevice();

  if (device->isConnected()) {
    Serial.println("connected.");
  } else {
    return false;
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
  }, [](byte error) {
    data[NEAR].humidity = INVALID_DATA;
  });
  sendCommand("get_prop", "\"TemperatureValue\"", [](JsonVariant result) {
    data[NEAR].temperature = result.as<int>();
  }, [](byte error) {
    data[NEAR].temperature = INVALID_DATA;
  });
  sendCommand("get_prop", "\"Humidifier_Gear\"", [](JsonVariant result) {
    gearState = result.as<byte>();
  });

  // this isn't nice, but what is...
  delay(TIMEOUT);
  render(NEAR);
  adjustHumidifier();
  Serial.println("Disconnecting.");
  device->disconnect();
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
}
