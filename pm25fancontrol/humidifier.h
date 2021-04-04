#include <airmath.h>

// c++ sucks so hard. inside a function the callback function is interpreted as a lambda which has a different type, so gotta do this outside, stupidly
// the volatile var even has to be global, heck.
volatile bool requiresSetup = false;
// make sure the light and notification sounds are off
void setupIfNecessary(VerboseBlockingMiioDevice *device) {
  device->get("Led_State", [requiresSetup](JsonVariant result) {
    if (result.as<int>() != 0) {
      requiresSetup = true;
    }
  });
  device->get("OnOff_State", [requiresSetup](JsonVariant result) {
    if (result.as<int>() != 1) {
      requiresSetup = true;
    }
  });
  // TODO wait?
  if (requiresSetup) {
    Serial.println("Humidifier looks uninitialized, setting up...");
    device->sendCommand("SetTipSound_Status", "0");
    device->sendCommand("Set_OnOff", "1");
    device->sendCommand("SetLedState", "0");
  }
}

class Humidifier : public VerboseBlockingMiioDevice {
  using VerboseBlockingMiioDevice::VerboseBlockingMiioDevice;
  public:
    float temperature = -1;
    float humidity = -1;

    byte setHumidityForDewPoint(float temperature, float desiredDewPoint) {
      // find highest comfortable humidity for temperature at the device
      // max comfortable is 60, max recommended 55, count down from there...
      byte humidity = 56;
      float dewPoint;
      do {
        humidity--;
        dewPoint = calculateDewPoint(temperature, humidity);
    //    Serial.printf("%.0f, %d: %.2f\n", humidifierData.temperature, humidity, humidifierData.dewPoint);
      } while (dewPoint > desiredDewPoint);

      this->sendCommand("Set_HumidifierGears", "4");
      this->sendCommand("Set_HumiValue", String(humidity));

      return humidity;
    };
};
