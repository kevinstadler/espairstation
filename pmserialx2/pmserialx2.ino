// SoftwareSerial.ino: Read PMS5003 sensor on SWSerial

#ifndef ESP32
#include <SoftwareSerial.h>
#endif
#include <PMserial.h> // Arduino library for PM sensors with serial interface
#define PMS_RX D3 // D4 is 2
#define PMS_TX 4 // D2 is 4

//SerialPM sensors[2] = { SerialPM(PMSx003, Serial), SerialPM(PMSx003, PMS_RX, PMS_TX) };
SerialPM pms = SerialPM(PMSx003, PMS_RX, PMS_TX);

void setup()
{
  Serial.begin(115200);
  Serial.println("Booted");
  pms.init();
//  sensors[1].init();
  Serial.println("Starting");
}

//void readSensor(SerialPM pms) {
void loop() {
  long t = millis();
  pms.read();
  Serial.println(millis() - t);
  if (pms) {
//    Serial.println(pms.has_particulate_matter());
//    Serial.println(pms.has_number_concentration());
//    Serial.println(pms.has_temperature_humidity());
//    Serial.println(pms.has_formaldehyde());
    // print formatted results
    Serial.printf("PM1.0 %2d, PM2.5 %2d, PM10 %2d [ug/m3]\n",
                  pms.pm01, pms.pm25, pms.pm10);

//    if (pms.has_number_concentration())
//      Serial.printf("N0.3 %4d, N0.5 %3d, N1.0 %2d, N2.5 %2d, N5.0 %2d, N10 %2d [#/100cc]\n",
//                    pms.n0p3, pms.n0p5, pms.n1p0, pms.n2p5, pms.n5p0, pms.n10p0);
  }
  else
  { // something went wrong
    Serial.println("something wrong");
    switch (pms.status)
    {
    case pms.OK: // should never come here
      break;     // included to compile without warnings
    case pms.ERROR_TIMEOUT:
      Serial.println(F(PMS_ERROR_TIMEOUT));
      break;
    case pms.ERROR_MSG_UNKNOWN:
      Serial.println(F(PMS_ERROR_MSG_UNKNOWN));
      break;
    case pms.ERROR_MSG_HEADER:
      Serial.println(F(PMS_ERROR_MSG_HEADER));
      break;
    case pms.ERROR_MSG_BODY:
      Serial.println(F(PMS_ERROR_MSG_BODY));
      break;
    case pms.ERROR_MSG_START:
      Serial.println(F(PMS_ERROR_MSG_START));
      break;
    case pms.ERROR_MSG_LENGTH:
      Serial.println(F(PMS_ERROR_MSG_LENGTH));
      break;
    case pms.ERROR_MSG_CKSUM:
      Serial.println(F(PMS_ERROR_MSG_CKSUM));
      break;
    case pms.ERROR_PMS_TYPE:
      Serial.println(F(PMS_ERROR_PMS_TYPE));
      break;
    }
  }
  delay(500);
}

void loop2() {
  for (byte i = 0; i < 2; i++) {
    // read the PM sensor
    Serial.printf("%d: ", i);
//    readSensor(sensors[i]);
  }
}
