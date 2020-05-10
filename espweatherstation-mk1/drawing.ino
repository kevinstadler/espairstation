#define AQIBOXSIZE 6
#define AQISECTIONHEIGHT (N_AQISTATIONS + 1) * AQIBOXSIZE + 3
#define AQIBOXAREAWIDTH (AQIBOXSIZE + 1) * N_AQIHISTORY

// OUT, HERE, NEAR, AQI
const byte POSITION[] = { 0, 30 + AQISECTIONHEIGHT, 60 + AQISECTIONHEIGHT, 30 };
const byte HEIGHT[] = { 30, 30, 30, AQISECTIONHEIGHT };

//  display.setFont(ucg_font_ncenR14_hr);
//  display.setFont(ucg_font_fub20_hf);
  // free universal regular/bold
  // transp/mono/height/8x8 + full/reduced/numbers
//  display.setFont(ucg_font_helvR24_tf); // helvetica
//  display.setFont(ucg_font_fur20_hf);
void setDefaultFont() { display.setFont(ucg_font_logisoso18_tf); }
void setSmallFont() { display.setFont(ucg_font_ncenR14_tf); }
// greek mu is 0x6d in ucg_font_symb12/14/18/24_tr
void setSymbolFont() { display.setFont(ucg_font_symb14_tr); }

char formatted[5];
char* formatFloat(float value, byte minimumWidth, byte decimals) {
  // stringLength < 6
  return dtostrf(value, minimumWidth, decimals, formatted);
}
char* formatFloat(float value, byte decimals) {
  return formatFloat(value, 0, decimals);
}
char* formatFloat(float value) {
  return formatFloat(value, 0, 0);
}

void draw(byte location) {
  switch(location) {
    case OUT: drawAQI();
    default: render(location); break;
  }
}

//  display.setColor(0, 0, 40, 80);
//  display.setColor(1, 80, 0, 40);
//  display.setColor(2, 255, 0, 255);
//  display.setColor(3, 0, 255, 255);
//  display.drawGradientBox(0, 0, display.getWidth(), display.getHeight());

void render(byte location) {
  setDefaultFont();
  byte y = POSITION[location] + HEIGHT[location] / 2;

  // clear
  display.setColor(0, 0, 0);
  display.drawBox(0, y - 16, display.getWidth(), HEIGHT[location]);

  // write
  display.setColor(100, 100, 110);
  if (location == HERE) {
    display.drawString(0, y, 0, "h:");
  } else if (location == NEAR) {
    display.drawString(0, y, 0, "x:");
  } else {
    display.drawString(0, y, 0, "o:");
  }
  byte x = 30;

  setTemperatureColor(data[location].temperature);
  x += display.drawString(x, y, 0, formatFloat(data[location].temperature, 1, 0));
  display.drawGlyph(x, y, 0, 0xb0); // degree sign

  x = 72;
  display.setPrintPos(x, y);
  setComfortColor(data[location].temperature, data[location].humidity);
  display.print((byte) data[location].humidity);
  display.print("%");

  // render uptime
//  display.setColor(64, 64, 64);
//  display.setFont(ucg_font_courR08_mr);
//  x = display.drawString(0, 120, 0, "uptime: ");
//  display.setPrintPos(x, 120);
//  display.print(millis() / 60000); // in minutes
}

void drawAQI() {
  // average over all available station data
  byte usedStations = 0;
  float pm25 = 0;
  for (byte i = 0; i < N_AQISTATIONS; i++) {
    if (pm25s[i][latestData[i]] > 0) { // FIXME dirty
      pm25 += pm25s[i][latestData[i]];
      usedStations++;
    }
  }

  // clear text
  display.setColor(0, 0, 0);
  byte x = AQIBOXAREAWIDTH + 4;
  display.drawBox(x, POSITION[3], display.getWidth() - x, HEIGHT[3]);

  if (usedStations == 0) {
    // TODO draw question mark?
    return;
  }
  pm25 = pm25 / usedStations;
  Serial.printf("PM25 average over %d stations is %.1fug/m3.\n", usedStations, pm25);

  // interpolate color between the two adjacent aqi categories
  Serial.println(getStepwiseLinearCatPos(pm25, CAQIPM25));
  setCAQIColor(getStepwiseLinearCatPos(pm25, CAQIPM25));
  // until AQI 150 (=PM25 55) one aqi index is < 1ug PM25, at 150 it switches to ~2mg per 1 aqi
  // so until 100ug we can render a formatted float, after we can just round to int
  byte decimals = pm25 < 100 ? 1 : 0;
  byte y = POSITION[3] + HEIGHT[3] / 2;
  setSmallFont();
  x += display.drawString(x, y, 0, formatFloat(pm25, 2, decimals));
  setSymbolFont();
  x += display.drawGlyph(x, y, 0, 0x6d);
  setSmallFont();
  display.drawString(x, y, 0, "g");

  // draw boxes
  // draw frame around first (default/closest) station
  display.setColor(80, 80, 80);
  display.drawFrame(0, POSITION[3], AQIBOXAREAWIDTH + 3, AQIBOXSIZE + 2);
  
  for (int i = 0; i < N_AQISTATIONS; i++) {
    for (int j = 0; j < N_AQIHISTORY; j++) {
      // draw right to left
      byte h = ( latestData[i] - j + N_AQIHISTORY ) % N_AQIHISTORY;
      if (pm25s[i][h] > 0) {
        setCAQIColor(getStepwiseLinearCatPos(pm25s[i][h], CAQIPM25));
      } else {
        display.setColor(0, 0, 0);
      }
      display.drawBox(AQIBOXAREAWIDTH - (j+1)*(AQIBOXSIZE+1), POSITION[3] + 1 + i*(AQIBOXSIZE + 1), AQIBOXSIZE, AQIBOXSIZE);
    }
  }

}
