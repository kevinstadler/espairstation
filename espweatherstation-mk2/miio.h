#include <ESPMiio.h>
// leave AT LEAST 150ms for a reliable connection, sometimes even 500 is not enough...
#define TIMEOUT 1000

AirData humidifierData = INVALID_AIRDATA;

bool tankEmpty = false;

// this is just the default IP, if no device is found there it will scan the local network
IPAddress *ip = new IPAddress(192, 168, 1, 2);

MiioDevice* createMiioDevice() {
  return new MiioDevice(ip, MIIO_TOKEN, TIMEOUT);
}
MiioDevice *device = createMiioDevice();

void findDevice() {
  IPAddress local = WiFi.localIP();

  // scan through the local network IP range to discover a MiIo device
  for (byte i = 2; i < 3; i++) {
//  for (byte i = 2; i < 11; i++) {
    if (i == local[3]) {
      continue; // don't try to connect to yourself
    }
    delete device;
    delete ip;
    ip = new IPAddress(local[0], local[1], local[2], i);
    Serial.print("Trying ");
    Serial.print(*ip);
    Serial.print("...");

    device = createMiioDevice();
    device->connect();
    delay(1000);
    if (device->isConnected()) {
      Serial.println("found!");
      break;
    } else {
      Serial.println("unavailable");
    }
  }
}

bool connectDevice();
bool sendCommand(String command, String jsonArgs, void (*callback)(JsonVariant result), void (*errorCallback)(byte errorCode));

bool sendCommand(String command, String jsonArgs, void (*callback)(JsonVariant result)) {
  return sendCommand(command, jsonArgs, callback, NULL);
}

bool sendCommand(String command, String jsonArgs) {
  return sendCommand(command, jsonArgs, NULL);
}

// see ESPMiio.h
const String MIIO_ERRORSTRING[5] = { "not connected", "timeout", "busy", "invalid response", "message creation failed" };

// 0 == waiting, 1 == OK, 2 == error
volatile byte responseStatus = 1;
bool sendCommand(String command, String jsonArgs, void (*callback)(JsonVariant result), void (*errorCallback)(byte errorCode)) {
  if (!connectDevice()) {
    Serial.println("ERROR: did not send command because no device connected.");
    return false;
  }
  Serial.print("Sending " + command + " " + jsonArgs + "...");
  byte waits = 0;
  while (device->isBusy()) {
    if (waits >= 20) {
      Serial.println("ERROR: failed to send command because UDP device is busy");
      return false;
    }
    Serial.print("waiting...");
    delay(200);
    waits++;
  }
  responseStatus = 0;
  if (device->send(command.c_str(), jsonArgs.c_str(),
      [command, jsonArgs, callback](MiioResponse response) {
        responseStatus = 1;
        Serial.println("response: " + response.getResult().as<String>() + ".");
        if (callback != NULL) {
          callback(response.getResult());
        }
      }, [errorCallback](byte error) {
        responseStatus = 2;
        Serial.println("error: " + MIIO_ERRORSTRING[error]);
        if (errorCallback != NULL) {
          errorCallback(error);
        }
      })) {
      Serial.print("sent, awaiting response..");
      while (responseStatus == 0) {
        Serial.print(".");
        delay(200);
      }
      delay(200); // one more for good measure...
      return responseStatus != 2;
    } else {
      Serial.println("failed.");
      return false;
    }
}

bool initializeHumidifier() {
  Serial.println("Setting up device...");
  return sendCommand("SetTipSound_Status", "0")
      && sendCommand("Set_OnOff", "1")
      && sendCommand("SetLedState", "0");
  // Set_HumiValue and Set_HumidifierGears will be called by adjustHumidifer()...
}

volatile bool requiresSetup = false;

bool connectDevice() {
  if (device->isConnected()) {
    return true;
  }
  Serial.print("Connecting to Miio device...");

  if (device->isBusy()) {
    Serial.print("busy...");
    delay(500);
  }
  responseStatus = 0;
  device->connect([](MiioError e) {
    responseStatus = 2;
    Serial.printf("failed (error %d)...", e);
  });
  // await connection
  while (!device->isConnected() && responseStatus == 0) {
    Serial.print(".");
    delay(200);
  }
  if (device->isConnected()) {
    Serial.println("connected.");
  } else {
    Serial.println("not connected, initiating scan.");
    findDevice();
  }
  if (device->isConnected()) {
    sendCommand("get_prop", "\"Led_State\"", [](JsonVariant result) {
      if (result.as<int>() != 0) {
        Serial.println("Device looks uninitiated, queueing setup.");
        requiresSetup = true;
      }
    });
    sendCommand("get_prop", "\"OnOff_State\"", [](JsonVariant result) {
      if (result.as<int>() != 1) {
        Serial.println("Device looks uninitiated, queueing setup.");
        requiresSetup = true;
      }
    });

    tankEmpty = false;
    sendCommand("get_prop", "\"waterstatus\"", [](JsonVariant result) {
      if (result.as<int>() != 1) { tankEmpty = true; }
    });
    sendCommand("get_prop", "\"watertankstatus\"", [](JsonVariant result) {
      if (result.as<int>() != 1) { tankEmpty = true; }
    });

    delay(TIMEOUT);
    if (requiresSetup) {
      if (initializeHumidifier()) {
        requiresSetup = false;
      }
      // TODO
  //      sendCommand("get_prop", "[\"OnOff_State\"]");
  //      sendCommand("get_prop", "[\"Led_State\"]");
    }
  }
  
  return device->isConnected();
}

void adjustHumidifier() {
  // find highest comfortable humidity for temperature at the device
  // max comfortable is 60, max recommended 55, count down from there...
  byte humidity = 56;
  do {
    humidity--;
    humidifierData.dewPoint = calculateDewPoint(humidifierData.temperature, humidity);
//    Serial.printf("%.0f, %d: %.2f\n", humidifierData.temperature, humidity, humidifierData.dewPoint);
  } while (humidifierData.dewPoint > DESIRED_DEWPOINT);
  Serial.printf("New ideal humidity value should be: %d.\n", humidity);

  // write correct value back for rendering...
  calculateDewPoint(humidifierData);

  sendCommand("Set_HumidifierGears", "4");
  sendCommand("Set_HumiValue", String(humidity));
}

void getHumidifierData() {
  if (!sendCommand("get_prop", "\"Humidity_Value\"", [](JsonVariant result) {
    humidifierData.humidity = result.as<int>();
  })) {
    Serial.println("Invalidating device humidity.");
    humidifierData.humidity = INVALID_DATA;
  }
  if (!sendCommand("get_prop", "\"TemperatureValue\"", [](JsonVariant result) {
    humidifierData.temperature = result.as<int>();
  })) {
    Serial.println("Invalidating device temperature.");
    humidifierData.temperature = INVALID_DATA;
  }

  if (isValid(calculateDewPoint(humidifierData))) {
    adjustHumidifier();
  }
  Serial.println("Disconnecting device.");
  device->disconnect();

  drawLocalData();
}
