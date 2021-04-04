#define INVALID_DATA NAN

bool isValid(float number) {
  return !isnan(number);
}

float calculateDewPoint(float temperature, float humidity) {
  if (!isValid(temperature) || !isValid(humidity)) {
    return INVALID_DATA;
  }
  float k;
  k = log(humidity/100) + (17.62 * temperature) / (243.12 + temperature);
  return 243.12 * k / (17.62 - k);
}

//float calculateDewPoint(AirData data) {
//#  // fill it in
//#  data.dewPoint = calculateDewPoint(data.temperature, data.humidity);
//#  return data.dewPoint;
//#}

// the second argument is an array of *inclusive* **upper** bounds
byte getStepwiseLinearCat(float value, const float *thresholds) {
  byte cat = 0;
  while (value > thresholds[cat]) {
    cat++;
  }
  return cat;
}

float mean(float v1, float v2) {
  return (v1 + v2) / 2;
}

float getStepwiseLinearCatPos(float value, const float *thresholds) {
  byte cat = getStepwiseLinearCat(value, thresholds);
  float catMean = mean(thresholds[cat], cat == 0 ? 0 : thresholds[cat - 1]);
  // in upper half: add up to .5 to cat
  if (value >= catMean) {
    return cat + 0.5 * (value - catMean) / (thresholds[cat] - catMean);
  }
  // in lower half: subtract up to .5 from cat, but don't go negative
  if (cat == 0) {
    return 0;
  }
  return cat - 0.5 * (catMean - value) / (catMean - thresholds[cat - 1]);
}
