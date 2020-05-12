// convert 24bit RGB to to 5 bits red, 6 bits green, 5 bits blue
const uint16_t CHANNELMASKS[] = { 0xF800, 0x7E0, 0x1F };

uint16_t ADAColor(uint32_t rgb) {
  return ((rgb >> 8) & CHANNELMASKS[0]) | ((rgb >> 5) & CHANNELMASKS[1]) | ((rgb >> 3) & CHANNELMASKS[2]);
}

#define BLACK 0x0000
#define WHITE 0xFFFF
#define RED ADAColor(0xFF0000)

// color schemes for comfort:
// comfort/discomfort r/g/b https://www.mrfixitbali.com/images/articleimages/dew-point-chart-compact.jpg
#define VERYDRY ADAColor(0xBBB3A6)
#define DRY ADAColor(0xAAAA66)
#define VERYCOMFY ADAColor(0x00E300) // this one's a bit shrill...
#define COMFY ADAColor(0xBBFF66) // that's a dull yellowish green, ok for humidity, not for temperature
#define OK ADAColor(0xFAFF8F)
#define YELLOWISH ADAColor(0xFFDD00)
#define ORANGE ADAColor(0xFF9900)
#define DARKORANGE ADAColor(0xFF5500)

#define MOON ADAColor(0xD0D0D0)
#define LIGHTCLOUD ADAColor(0x999999)
#define DARKCLOUD ADAColor(0x555555)
#define RAINDROPS ADAColor(0x0044FF)

uint16_t getColor(float value, const float *thresholds, const uint16_t *colors) {
  return colors[getStepwiseLinearCat(value, thresholds)];
}

uint16_t mixColors(uint16_t color1, uint16_t color2, float secondComponent) {
  return ((uint16_t)((color1 & CHANNELMASKS[0]) * (1 - secondComponent) + (color2 & CHANNELMASKS[0]) * secondComponent) & CHANNELMASKS[0])
       | ((uint16_t)((color1 & CHANNELMASKS[1]) * (1 - secondComponent) + (color2 & CHANNELMASKS[1]) * secondComponent) & CHANNELMASKS[1])
       | ((uint16_t)((color1 & CHANNELMASKS[2]) * (1 - secondComponent) + (color2 & CHANNELMASKS[2]) * secondComponent) & CHANNELMASKS[2]);
}

uint16_t mixColors(float value, const float *thresholds, const uint16_t *colors) {
  float catPos = getStepwiseLinearCatPos(value, thresholds);
  byte cat = (byte) catPos;
  return mixColors(colors[cat], colors[cat + 1], catPos - cat);
}
