#include <ESPMiio.h>
// leave AT LEAST 150ms for a reliable connection, sometimes even 500 is not enough...
#define MIIO_TIMEOUT 1000

class VerboseBlockingMiioDevice : public MiioDevice {
public:
  VerboseBlockingMiioDevice(IPAddress* ip, std::string token, Stream* logTarget, uint16_t timeout);
  bool awaitConnection();
  bool get(String prop, void (*callback)(JsonVariant result));
  bool sendCommand(String command, String jsonArgs, void (*callback)(JsonVariant result), void (*errorCallback)(byte errorCode));
  bool sendCommand(String command, String jsonArgs, void (*callback)(JsonVariant result));
  bool sendCommand(String command, String jsonArgs);
  void log(String msg);
private:
  // 0 == waiting, 1 == OK, 2 == error
  volatile byte responseStatus = 1;
  Stream* logTarget;
};

VerboseBlockingMiioDevice::VerboseBlockingMiioDevice(IPAddress* ip, std::string token, Stream* logTarget = nullptr, uint16_t timeout = MIIO_TIMEOUT) : MiioDevice(ip, token, timeout) {
  this->logTarget = logTarget;
};

bool VerboseBlockingMiioDevice::get(String prop, void (*callback)(JsonVariant result)) {
  return this->sendCommand("get_prop", "\"" + prop + "\"", callback);
}

void VerboseBlockingMiioDevice::log(String msg) {
  if (this->logTarget != nullptr) {
    this->logTarget->println(msg);
  }
}

bool VerboseBlockingMiioDevice::awaitConnection() {
  if (this->isConnected()) {
    return true;
  }
  this->log("Connecting to Miio device...");

  if (this->isBusy()) {
    this->log("busy...");
    delay(500);
  }
  this->responseStatus = 0;
  this->connect([this](MiioError e) {
    this->responseStatus = 2;
    Serial.printf("failed (error %d)...", e);
  });
  // await connection
  while (!this->isConnected() && this->responseStatus == 0) {
    delay(200);
  }
  return this->isConnected();
}

bool VerboseBlockingMiioDevice::sendCommand(String command, String jsonArgs, void (*callback)(JsonVariant result)) {
  return this->sendCommand(command, jsonArgs, callback, NULL);
}
bool VerboseBlockingMiioDevice::sendCommand(String command, String jsonArgs) {
  return this->sendCommand(command, jsonArgs, NULL);
}

// see ESPMiio.h
const String MIIO_ERRORSTRING[5] = { "not connected", "timeout", "busy", "invalid response", "message creation failed" };

bool VerboseBlockingMiioDevice::sendCommand(String command, String jsonArgs, void (*callback)(JsonVariant result), void (*errorCallback)(byte errorCode)) {
  if (!this->isConnected()) {
    Serial.println("ERROR: did not send command because no device connected.");
    return false;
  }
  Serial.print("Sending " + command + " " + jsonArgs + "...");
  byte waits = 0;
  while (this->isBusy()) {
    if (waits >= 20) {
      this->log("ERROR: failed to send command because UDP device is busy");
      return false;
    }
    this->log("waiting...");
    delay(200);
    waits++;
  }
  this->responseStatus = 0;
  if (this->send(command.c_str(), jsonArgs.c_str(),
      [this, command, jsonArgs, callback](MiioResponse response) {
        this->responseStatus = 1;
        this->log("response: " + response.getResult().as<String>() + ".");
        if (callback != NULL) {
          callback(response.getResult());
        }
      }, [this, errorCallback](byte error) {
        this->responseStatus = 2;
        this->log("error: " + MIIO_ERRORSTRING[error]);
        if (errorCallback != NULL) {
          errorCallback(error);
        }
      })) {
      this->log("sent, awaiting response..");
      while (this->responseStatus == 0) {
        delay(200);
      }
      delay(200); // one more for good measure...
      return this->responseStatus != 2;
    } else {
      this->log("failed.");
      return false;
    }
}
