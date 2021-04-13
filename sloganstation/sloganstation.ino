#include <wifi.h>
long RTCmillis() {
  // system_get_rtc_time() returns us as a uint32_t
  // system_rtc_clock_cali_proc() >> 12 is often SIX
  return (system_get_rtc_time() * (system_rtc_clock_cali_proc() >> 12)) / 1000;
}

#include "openaq.h"

#define NEOMATRIX_PIN D3
#include "neomatrix.h"

#define MOTIONSENSOR_PIN D7
#include "motionsensor.h"
#define SCREENONTIME 60e3 // stay on for one minute

#define RELAY_PIN D1

void setup() {
  // UNSET or LOW = 4.7V on the relay, relay led is off
  // HIGH = 3.3V on it?? led is on
  pinMode(RELAY_PIN, OUTPUT);
  initMatrix(10);
  Serial.begin(115200);
  Serial.println();
  getNewData();
}

void wakeupCallback() {
  Serial.println(RTCmillis());
  Serial.flush();
}

// max light sleep is 268s ~ 4 1/2 minutes
void lightSleep(uint32_t sleepMs = 268435) {
  Serial.print(RTCmillis());
  Serial.print(" -> ");
  Serial.flush();
  // ...and go to sleep
  WiFi.mode(WIFI_OFF); // without this light sleep causes a soft reset
  extern os_timer_t *timer_list;
  timer_list = nullptr;
  wifi_fpm_set_sleep_type(LIGHT_SLEEP_T);
  wifi_fpm_open();
  gpio_pin_wakeup_enable(MOTIONSENSOR_PIN, GPIO_PIN_INTR_HILEVEL);
  wifi_fpm_set_wakeup_cb(wakeupCallback);
  wifi_fpm_do_sleep(sleepMs * 1000);
  delay(sleepMs + 1);
}

// render on first loop
byte inactivityCount = 1;
bool activity = true;

// [X] no activity, already off (inactivityCount > 0) -> do nothing
// [X] no activity, was on (inactivityCount == 0) -> turn off
// [X] activity, was off (inactivityCount > 0) -> screen on, render
// [X] activity, was on (inactivityCount == 0) -> stay on, do nothing
// [ ] always: check time elapsed, get new data -- if new data + screen is on -> don't sleep, go to start of loop

void loop() {
  uint32_t sleepTime = 268435;
  if (activity) {
    activity = false;
    // if it was previously off, turn on
    if (inactivityCount > 0) {
      // was off, turn on
      if (inactivityCount >= 10) {
        // reenable relay and init display
  //      digitalWrite(RELAY_PIN, LOW);
        //initMatrix(10);
      }
      drawPM25();
      drawDoggie(lastNtpTime);
      matrix.show();
    }
    inactivityCount = 0;
    // only keep screen on for 30s
    sleepTime = SCREENONTIME;

  } else {
    // woke up but PIR is low, i.e. sleep timed out without activity

    // keep on counting, but cap counter at 255 to avoid overflow
    switch (inactivityCount) {
      case 255:
        break;
      case 0:
        // first inactivity -> turn off
        matrix.clear();
        matrix.show();
      default:
        inactivityCount++;
        if (inactivityCount == 10) {
          // TODO turn relay off!
    //      digitalWrite(RELAY_PIN, HIGH);
        }
    }
  }

  // TODO check if time has elapsed, if so go in here to update data anyway (but don't turn screen on)
//  unsigned long now = lastNtpTime + (RTCmillis() - lastNtpTimeMillis) / 1000;
  if (RTCmillis() - lastNtpTimeMillis > 1800e3) { // 30 minutes
    Serial.println("Some time elapsed, getting new data");
    if (getNewData() && inactivityCount == 0) {
      matrix.clear();
      drawPM25();
      drawDoggie(lastNtpTime);
      matrix.show();
    }
  }

  // TODO check if pin is high?
  while (digitalRead(MOTIONSENSOR_PIN)) {
    delay(1000);
  }
  // sleep for desired time
  lightSleep(sleepTime);
  activity = digitalRead(MOTIONSENSOR_PIN);
}
