AirData outdoorData = INVALID_AIRDATA;
float feelsLike = INVALID_DATA;
float feelsLikeShade = INVALID_DATA;
String currentIcon;

#define FORECAST 2
float forecastDay[FORECAST] = { INVALID_DATA, INVALID_DATA };
float forecastNight[FORECAST] = { INVALID_DATA, INVALID_DATA };
String forecastIcon[FORECAST];

unsigned long currentSyncTime;
unsigned long forecastSyncTime;

// API comparison: https://rapidapi.com/blog/access-global-weather-data-with-these-weather-apis/
// OpenWeatherMap: 'feels like' does not take sun/shade into account...

enum Icon {
  CLEAR = 1,
  FEWCLOUDS = 2,       // 1-2 oktas, 11-25% on OpenWeatherMap
  SCATTEREDCLOUDS = 3, // 3-4 oktas, 25-50%
  OVERCAST = 4,        // broken clouds = 5-7 oktas, 51-84%, 85-100%
  SHOWERS = 9,
  RAIN = 10,
  THUNDERSTORM = 11,
  SNOW = 13,
  MIST = 50
};

// 20+ icons...: https://developer.accuweather.com/weather-icons
// inclusive upper bounds
float accuWeatherIconThresholds[] = { 2, 4, 5, 6, 8, 11, 14, 17, 18, 24, 29, 30, 31, 32,
  34, 36, 37, 38, 40, 42, 44 };
Icon openWeatherMapIcons[] = { CLEAR, FEWCLOUDS, MIST, SCATTEREDCLOUDS, OVERCAST, MIST, SHOWERS, THUNDERSTORM, RAIN, SNOW, RAIN, CLEAR, SNOW, MIST,
  CLEAR, FEWCLOUDS, MIST, SCATTEREDCLOUDS, SHOWERS, THUNDERSTORM, SNOW };

String accuWeatherToOpenWeatherMapIcons(byte accuWeather) {
  String timeOfDay = accuWeather <= 32 ? "d" : "n";
  return String(openWeatherMapIcons[getStepwiseLinearCat(accuWeather, accuWeatherIconThresholds)]) + timeOfDay;
}

void getWeather() {
  if (hasOneHourExpired(currentSyncTime)) {
//    if (false) {
    Serial.print("Getting AccuWeather data...");
    JsonVariant data = getJSON("http://dataservice.accuweather.com/currentconditions/v1/2333573?apikey=U12BJNak6q62kUgJbMsYLIkgHbQRDOJA&details=true");
    currentSyncTime = data[0]["EpochTime"];
    if (currentSyncTime > 0) {
      Serial.printf("received, from %d minutes ago.\n", (now - currentSyncTime) / 60);
      currentIcon = accuWeatherToOpenWeatherMapIcons(data[0]["WeatherIcon"].as<byte>());
      outdoorData.temperature = data[0]["Temperature"]["Metric"]["Value"].as<float>();
      outdoorData.humidity = data[0]["RelativeHumidity"].as<float>();
      outdoorData.dewPoint = data[0]["DewPoint"]["Metric"]["Value"].as<float>();
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

  // incremental draw
  drawWeather();

  if (hasOneHourExpired(forecastSyncTime) || !isValid(feelsLike)) {
    Serial.print("Getting OpenWeatherMap data...");
    // https://openweathermap.org/api/one-call-api
    JsonVariant data = getJSON("http://api.openweathermap.org/data/2.5/onecall?lat=25.06&lon=102.7&units=metric&appid=" + OPENWEATHERMAP_KEY);
    forecastSyncTime = data["current"]["dt"].as<unsigned long>();
    if (forecastSyncTime > 0) {
      Serial.printf("received, from %d minutes ago.\n", (now - forecastSyncTime) / 60);
      currentIcon = data["current"]["weather"][0]["icon"].as<String>();
      outdoorData.temperature = data["current"]["temp"].as<float>();
      outdoorData.humidity = data["current"]["humidity"].as<float>();
      outdoorData.dewPoint = data["current"]["dew_point"].as<float>();
      if (!isValid(feelsLike)) {
        feelsLike = max(outdoorData.temperature, data["current"]["feels_like"].as<float>());
        feelsLikeShade = min(outdoorData.temperature, data["current"]["feels_like"].as<float>());
      }

      // if the time of the first forecast element is actually before
      // now (i.e. forecast for (the rest of) today), skip it
      // (adding 12 hours so that until 12 noon, the forecast for today will be displayed
      bool forecastOffset = data["daily"][0]["dt"].as<unsigned long>() + 60*60*12 < forecastSyncTime;
      for (byte i = 0; i < FORECAST; i++) {
        forecastDay[i] = data["daily"][forecastOffset + i]["temp"]["day"].as<float>();
        forecastNight[i] = data["daily"][forecastOffset + i]["temp"]["night"].as<float>();
        forecastIcon[i] = data["daily"][forecastOffset + i]["weather"][0]["icon"].as<String>();
      }
  //    data["hourly"][0]["temp"];
  //    data["hourly"][0]["feels_like"];
  //    data["hourly"][0]["weather"]["icon"];
    } else {
      Serial.println("OpenWeatherMap call failed, getting forecast from AccuWeather...");
      JsonVariant data = getJSON("http://dataservice.accuweather.com/forecasts/v1/daily/5day/2333573?apikey=U12BJNak6q62kUgJbMsYLIkgHbQRDOJA&details=true&metric=true");
      // this can be up to an hour later actually!
      forecastSyncTime = data["Headline"]["EffectiveEpochDate"].as<unsigned long>();
      if (forecastSyncTime > 0) {
        Serial.printf("received forecast, from %d minutes ago.\n", (now - forecastSyncTime) / 60);
        for (byte i = 0; i < FORECAST; i++) {
          // could do RealFeelTemperature instead...
//          Serial.println(data["DailyForecasts"][i].as<String>());
          forecastDay[i] = data["DailyForecasts"][i]["Temperature"]["Maximum"]["Value"].as<float>();
          forecastNight[i] = data["DailyForecasts"][i]["Temperature"]["Minimum"]["Value"].as<float>();
          forecastIcon[i] = accuWeatherToOpenWeatherMapIcons(data["DailyForecasts"][i]["Day"]["Icon"].as<byte>());
        }
      }
    }
  }

  drawWeather();
  drawLocalData();
}
