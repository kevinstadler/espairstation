typedef const long color_t;

// AQI category names according to EPA
//#define GREEN 0x00E400
//#define YELLOW 0xFFFF00
//#define ORANGE 0xFF7E00
//#define RED 0xFF0000
//#define PURPLE 0x99004C
//#define MAROON 0x7EFF23
//color_t aqiColors[] = { GREEN, YELLOW, ORANGE, RED, PURPLE, MAROON };

color_t caqiColors[] = { 0x79BC6A, 0xBBCF4C, 0xEEC20B, 0xF29305, 0xE8416F };

// color schemes for comfort:
// comfort/discomfort r/g/b https://www.mrfixitbali.com/images/articleimages/dew-point-chart-compact.jpg
#define DRY 0xAAAA66
#define VERYCOMFY 0x00E400 // this one's a bit shrill tbh...
#define COMFY 0xBBFF66
#define OK 0xFAFF8F
#define UNCOMFY 0xFFDD00
#define QUITEUNCOMFY 0xFF9900
#define VERYUNCOMFY 0xFF5500
#define SEVEREUNCOMFY 0x7EFF23
// humidity:g/w/b https://www.metabunk.org/attachments/20161022-103944-b5wqu-jpg.22222/
// categories: dry, verycomfy, comfy, ok, uncomfy, quiteuncomfy, veryuncomfy, severeuncomfy
color_t humidityColors[] = { DRY, VERYCOMFY, COMFY, OK, UNCOMFY, QUITEUNCOMFY, VERYUNCOMFY, SEVEREUNCOMFY };

#define WHITE 0xFFFFFF
#define VERYCOLD 0x0000AA
#define COLD 0x4444CC
#define SLIGHTLYCOLD 0x7777FF
color_t temperatureColors[] = { WHITE, VERYCOLD, COLD, SLIGHTLYCOLD, VERYCOMFY, COMFY, OK, UNCOMFY, QUITEUNCOMFY, VERYUNCOMFY, SEVEREUNCOMFY };
// upper (inclusive) boundaries
const float temperatureThresholds[] = { -100, 4, 10, 18, 20, 25, 28, 31, 34, 37, 100 };

byte R(const long color) {
  return (color >> 16) & 0xFF;
}
byte G(const long color) {
  return (color >> 8) & 0xFF;
}
byte B(const long color) {
  return color & 0xFF;
}

void setColor(const long color) {
  display.setColor(R(color), G(color), B(color));
}

void setMixedColor(const long color1, const long color2, float secondComponent) {
  byte r = (1 - secondComponent) * R(color1) + secondComponent * R(color2);
  byte g = (1 - secondComponent) * G(color1) + secondComponent * G(color2);
  byte b = (1 - secondComponent) * B(color1) + secondComponent * B(color2);
  display.setColor(r, g, b);
}

void setCAQIColor(float caqiCatPos) {
  byte caqiCat = caqiCatPos;
  caqiCatPos -= caqiCat;
  setMixedColor(caqiColors[caqiCat], caqiColors[caqiCat + 1], caqiCatPos);
}

void setTemperatureColor(float temperature) {
  setColor(temperatureColors[getStepwiseLinearCat(temperature, temperatureThresholds)]);
}

void setComfortColor(float temperature, float humidity) {
  byte perception = dht.computePerception(temperature, humidity, false);
  setColor(humidityColors[perception]);
}
