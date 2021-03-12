#include <Wire.h>
#include "SSD1306Wire.h"

SSD1306Wire display(0x3c, SDA, SCL);

#include <SoftwareSerial.h>
SoftwareSerial soft(D5, D7); // RX TX

#include "PMS.h"
PMS pms(soft);
PMS::DATA pmdata;

void setup() {
  Serial.begin(115200);
  soft.begin(9600);
  display.init();
  display.flipScreenVertically();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10);
}

#define NDATA 1024
uint16_t data[NDATA];
unsigned long times[NDATA];
uint16_t latest = 0;

// ATTENTION the display thinks it's 128x64, but only the center is visible to me
// top left: (32, 16) bottom right: (95, 63)

// for 10 reads show by measure
byte counter;
void loop() {
  display.clear();
  display.setColor(WHITE);
//  display.drawLine(32, 16, 32, 63); // left
//  display.drawLine(95, 16, 95, 63); // right
//  display.drawLine(32, 16, 95, 16); // top
//  display.drawLine(32, 63, 95, 63); // bottom

  if (!pms.readUntil(pmdata)) {
    Serial.println("error");
    display.drawString(32, 16, "error");
  } else {
    latest++;
    data[latest] = pmdata.PM_AE_UG_2_5;
    times[latest] = millis();
    display.drawString(32, 16, String(data[latest]));
  }

  byte measurementsPerPixel = counter / 10 + 1;
  byte firstIndex = latest;
  for (byte i = 0; i < 64; i++) {
    // i is the pixel index!
    uint16_t sum = 0;
    for (byte j = 0; j < measurementsPerPixel; j++) {
      firstIndex = (firstIndex - 1 + NDATA) % NDATA;
      sum += data[firstIndex];
    }
    // divide by 4
    display.setPixel(95 - i, 63 - sum / (measurementsPerPixel*2));
  }
  display.drawString(64, 16, String((times[latest] - times[firstIndex])/1000));
  // make 20, 40, 60, 80 markers
  display.setPixel(95, 63);
  display.setPixel(95, 58);
  display.setPixel(95, 53);//10
  display.setPixel(95, 48);
  display.setPixel(95, 43);//20
  display.setPixel(95, 38);
  display.setPixel(95, 33);//30
  display.display();
  counter = (counter + 1) % 50;
}
