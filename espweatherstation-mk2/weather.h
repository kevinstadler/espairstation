AirData outdoorData = INVALID_AIRDATA;
float feelsLike = INVALID_DATA;
float feelsLikeShade = INVALID_DATA;
String currentIcon;

#define FORECAST 2
int forecastDay[FORECAST];
int forecastNight[FORECAST];
String forecastIcon[FORECAST];

unsigned long accuSyncTime;
unsigned long openSyncTime;

// API comparison: https://rapidapi.com/blog/access-global-weather-data-with-these-weather-apis/
// OpenWeatherMap: 'feels like' does not take sun/shade into account...

void getWeather() {
  if (hasOneHourExpired(accuSyncTime)) {
//    if (false) {
    Serial.print("Getting AccuWeather data...");
    JsonVariant data = getJSON("http://dataservice.accuweather.com/currentconditions/v1/2333573?apikey=U12BJNak6q62kUgJbMsYLIkgHbQRDOJA&details=true");
    accuSyncTime = data[0]["EpochTime"];
    if (accuSyncTime > 0) {
      Serial.printf("received, from %d minutes ago.\n", (now - accuSyncTime) / 60);
      // 20+ icons...: https://developer.accuweather.com/weather-icons
      outdoorData.temperature = data[0]["Temperature"]["Metric"]["Value"].as<float>();
      feelsLike = data[0]["RealFeelTemperature"]["Metric"]["Value"].as<float>();
      feelsLikeShade = data[0]["RealFeelTemperatureShade"]["Metric"]["Value"].as<float>();
    //  http://dataservice.accuweather.com/forecasts/v1/hourly/12hour/2333573?apikey=U12BJNak6q62kUgJbMsYLIkgHbQRDOJA&details=true&metric=true
    // http://dataservice.accuweather.com/forecasts/v1/daily/5day/2333573?apikey=U12BJNak6q62kUgJbMsYLIkgHbQRDOJA&details=true&metric=true
    } else {
      Serial.println("Invalidating 'feels like'...");
      feelsLike = INVALID_DATA;
      feelsLikeShade = INVALID_DATA;
    }
  }

  if (hasOneHourExpired(openSyncTime) || !isValid(feelsLike)) {
    Serial.print("Getting OpenWeatherMap data...");
    // https://openweathermap.org/api/one-call-api
    JsonVariant data = getJSON("http://api.openweathermap.org/data/2.5/onecall?lat=25.06&lon=102.7&units=metric&appid=" + OPENWEATHERMAP_KEY);
    openSyncTime = data["current"]["dt"].as<unsigned long>();
    if (openSyncTime > 0) {
      Serial.printf("received, from %d minutes ago.\n", (now - openSyncTime) / 60);
      currentIcon = data["current"]["weather"][0]["icon"].as<String>();
      outdoorData.temperature = data["current"]["temp"].as<float>();
      outdoorData.humidity = data["current"]["humidity"].as<float>();
      outdoorData.dewPoint = data["current"]["dew_point"].as<float>();
      if (!isValid(feelsLike)) {
        feelsLike = max(outdoorData.temperature, data["current"]["feels_like"].as<float>());
        feelsLikeShade = min(outdoorData.temperature, data["current"]["feels_like"].as<float>());
      }
  
      for (byte i = 0; i < FORECAST; i++) {
        forecastDay[i] = data["daily"][i]["temp"]["day"].as<float>();
        forecastNight[i] = data["daily"][i]["temp"]["night"].as<float>();
        forecastIcon[i] = data["daily"][i]["weather"][0]["icon"].as<String>();
      }
  //    data["hourly"][0]["temp"];
  //    data["hourly"][0]["feels_like"];
  //    data["hourly"][0]["weather"]["icon"];
    }
  }

  drawWeather();
  drawLocalData();
}
