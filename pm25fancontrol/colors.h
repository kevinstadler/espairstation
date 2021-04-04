byte BLACK[] = { 0, 0, 0 };
byte WHITE[] = { 255, 255, 255 };

const float AQI[] = { 12, 35, 55, 150, 250, 350, 500 };
// original EPA-defined colors: https://www.airnow.gov/sites/default/files/2020-05/aqi-technical-assistance-document-sept2018.pdf
const byte AQICOLORS[][3] = {
  { 0, 228, 0 },
  { 255, 255, 0 },
  { 255, 126, 0 },
  { 255, 0, 0 },
  { 143, 63, 151 },
  { 126, 0, 35 }
};
