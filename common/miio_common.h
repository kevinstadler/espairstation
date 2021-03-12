#include <ESPMiio.h>
// leave AT LEAST 150ms for a reliable connection, sometimes even 500 is not enough...
#define TIMEOUT 1000

MiioDevice* createMiioDevice() {
  return new MiioDevice(ip, MIIO_TOKEN, TIMEOUT);
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

