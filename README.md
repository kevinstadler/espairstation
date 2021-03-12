## espairstation

Several ESP8266/ESP32 Arduino projects for collecting and displaying air temperature/humidity/pollution data from local and remote sensors.

Sub-projects and corresponding hardware:

* `sloganstation` (aka `espweatherstation-mk3`) -- expanded version of `mk2` that uses a `NeoMatrix` display
  * local
    * NeoMatrix LED
    * TEMT6000 for automatic dimming (see [AniMatrix](https://github.com/kevinstadler/AniMatrix))
    * PIR sensor triggers screen wake (on time/sleep is fixed, not dependent on PIR going down)
    * SHT30 shield for local temperature/humidity
  * network
    * MiIO humidifier for reading temperature/humidity data and controlling device state
    * [OpenAQ](https://openaq.org) JSON API for outdoor pollution data (raw)
    * AccuWeather/OpenWeatherMap for outdoor measurements + forecasting
* `espweatherstation-mk2`
  * local
    * SSD1351-based 128x128px RGB OLED display 
    * SR-501 PIR sensor for dynamically turning the screen on/off (based on SR-501's hardware time setting)
    * DHT22 temperature/humidity sensor for local temperature/humidity
  * network
    * MiIO humidifier for reading temperature/humidity data and controlling device state
    * [AQICN](http://aqicn.org) JSON API for outdoor pollution (reconstructed)/temperature/humidity data
* `espweatherstation-mk1`
  * original code, abandoned in favour of `mk2`
* `pm25fancontrol` turns a MiIO smart plug on/off based on local (and outside) PM2.5 concentration readings
  * local
    * Plantower PMS5003
    * RGB led (ground/pwm-controlled)
    * TEMT6000 for LED brightness dimming?
    * PIR sensor for waking (turning led on/off)
  * network
    * MiIO smart plug
    * outside PM25 readings
* `PIRtest` for testing/comparing PIR sensor sensitivity
  * local
    * SR-501 sensitivity/time/trigger-adjustable PIR sensor
    * ? PIR sensor
    * SR-602 PIR sensor
    * `8x32` MAX7219 LED (https://www.makerguides.com/max7219-led-dot-matrix-display-arduino-tutorial/)
* `PortablePM25Screen` reads and graphs PM2.5 concentrations on a 64x48 pixel OLED shield
  * PMS5003 sensor (uses fu-hsi's `PMS.h`)
  * D1 Mini [OLED 0.66" shield](https://www.wemos.cc/en/latest/d1_mini_shield/oled_0_66.html)
    * this display was a pain to get running as most libraries are written for the 128x64 pixel version, `SSD1306Wire` library worked in the end but with funny pixel offsets
* `pmserialx2` reads PM2.5 concentration from 2 connected sensors for comparison (at some point I also wrote them to SPIFFS for plotting+comparison, wonder where that code went...)
  * 2 PMS5003 sensors (uses the always-blocking ``PMserial.h``)
* [`srtNeoMatrix`](https://github.com/kevinstadler/srtNeoMatrix) (dedicated repository): displays subtitles from a .srt file read from SPIFFS on a NeoMatrix in real time
  * NeoMatrix
  * TEMT6000 for automatic dimming (see [AniMatrix](https://github.com/kevinstadler/AniMatrix))

## Hardware

### Micro-controller chips

* Wemos D1 Mini (ESP8266): great little chip, compact, but requires soldering. [Lots of great shields](https://www.wemos.cc/en/latest/d1_mini_shield/index.html) available for it but 'stacked' female pins can lead to unstable connections.
  * definite pin usage reference: https://randomnerdtutorials.com/esp8266-pinout-reference-gpios/
* LoL1n NodeMCU 1.0 (ESP-12E): nice dev board but can only flash at 115200 baud (SLOW) and something about non-standard crystal frequencies etc makes serial output garbled/unusable (see esp8266/Arduino#4005)

### NeoMatrix display

https://learn.adafruit.com/adafruit-neopixel-uberguide/best-practices

* does not light up without a 1000mA cap
* unlike stated elsewhere, controlling the 5V powered matrix from the 3.3V ESP8266 works fine without the need for a logic level converter

### PM2.5 sensors

All laser-based, 5V power, 3.3V logic level devices communicating via Serial.

* [Plantower PMS 7003](https://www.espruino.com/datasheets/PMS7003.pdf) +-10ug accuracy, 100mA usage, .2mA standby, has a sleep pin but does not provide 3.3V out. ([Arduino driver],(https://www.espruino.com/PMS7003), [AQICN review](https://aqicn.org/sensor/pms5003-7003/), [Plantower product website](http://www.plantower.com/list/?6_1.html): 7003M model has fan+exhaust at the top, normal/P/I modals 90 opposed on the side?)
  * also found a (slightly) cheaper copy of the Plantower with the exact same specs/protocol, labelled *G7*
* [Honeywell HPMA115SO-XXX](https://sensing.honeywell.com/honeywell-sensing-particulate-hpm-series-datasheet-32322550) +-15ug accuracy, 600mA surge voltage, 80mA usage, 20mA standby, has a 3.3V 100mA output pin.... ([Arduino driver](https://github.com/felixgalindo/HPMA115S0))

Laboratory experiment comparisons:

* https://uwspace.uwaterloo.ca/bitstream/handle/10012/12776/Tan_Ben.pdf?sequence=5&isAllowed=y

#### PMS\*003 libraries

* PMSSerial (`PMSerial.h`) takes a Serial object for HW and has SoftwareSerial 'built-in', is always in passive mode(?), has an OLED example sketch, read commands are generally blocking.

* fu-hsi's [PMS Library](https://github.com/fu-hsi/PMS) (`PMS.h`) takes a Serial object as argument and has methods for turning on/off passive mode and provides both blocking and non-blocking methods. Because the library uses blocking calls sparingly there is an issue [using the library with SoftwareSerial](https://github.com/fu-hsi/PMS/issues/14) where it is sometimes necessary to manually `flush()` the software serial port before executing a command, otherwise it might not be transmitted properly (`AltSoftSerial` and `NeoSWSerial` don't have this problem, but neither are supported on the ESP8266).

### PIR sensors

### SR-501 (nice chunky fresnel lens)

Setting the jumper to "repeat trigger" and the delay quite high (15+ seconds) allows you to control the screen (powerUp/powerDown) via a `CHANGE` interrupt on the data pin. The sensor takes about a minute to adjust to baseline lighting, so only attach the interrupt at the end of the `setup()` return after waiting for about a minute.

All other sensors have a fixed sensitivity and trigger `HIGH` for 1-2 sec before going `LOW` again for at least 5 sec.

### AM-312

spec says 100deg angle, 3-5 meter range, lense smaller than on SR-501 but seems to work very similar to SR-501 on highest-sensitivity and lowest timer setting

### SR-602

The small and clunky lens makes it useless except for narrow spaces or close-up detection. Never figured out what the extra 'photosensitivity' pins are good for.

### temperature/humidity sensors

#### SHT30

MUCH smaller than DHT sensors, also available as a super flat [D1 Mini shield](https://www.wemos.cc/en/latest/d1_mini_shield/sht30.html) (pins `D1` and `D2` for I2C, addresses `0x44` and `0x45`).

#### DHT22

Is constantly reading data when it's only required to update every 5 minutes or so, maybe possible to supply voltage via one of the digital Pins?

TODO: need to figure out consumption of the DHT22 and the maximum current draw for the digital pins.

* https://forum.arduino.cc/index.php?topic=332915.0
* https://forum.mysensors.org/topic/323/powering-sensors-through-the-digital-pins/3

### MiIo / how to get your MiIO token

The easiest way to extract current tokens for all your connected MiIO/Xiaomi Home devices is this python script, which simply uses your MiHome login data to query the Xiaomi Cloud API for tokens: https://github.com/PiotrMachowski/Xiaomi-cloud-tokens-extractor

For testing/debugging/figuring out commands for MiIO devices the best command seems to be [python-miio](https://github.com/rytilahti/python-miio) (use the --debug option to inspect the underlying JSON of each implemented command).

For communicating with the devices on Arduino I used ar2rus' experimental [ESPMiIO](https://github.com/ar2rus/ESPMiIO/) library with some modifications (see my [fork](https://github.com/kevinstadler/ESPMiIO/)).

<!-- My device model is a Xiaomi Humidifier 2(?) aka `deerma.humidifier.mjjsq`. -->

~~For obtaining the MiIo token required to connect to the device I followed https://github.com/Maxmudjon/com.xiaomi-miio/blob/master/docs/obtain_token.md but none of the elegant methods worked: Arduino backup was a pain, [even with setting a backup desktop password](https://android.stackexchange.com/questions/116439/adb-backup-command-on-non-rooted-device-creates-an-empty-backup-file) it just didn't work, the auto-token obtained via the `miio` command line tool would also expire after connecting the device to Wifi, so ended up installing the verbosely logging Xiaomi Home version instead.~~

### AQI sources

#### aqicn.com/WAQI JSON API

Up-to-date but only gives EPA-standardised air quality indices and only for the current time (no history/backlog)

See [hackster.io: ESP8266 AQI Display](https://www.hackster.io/arkhan/esp8266-aqi-display-25bba7)

#### OpenAQ JSON API

No weather data but otherwise much better, raw measurements, also up to date and no API key needed

https://openaq.org

#### [pm25.in](http://www.pm25.in)

No longer maintained/no response to API key request. 'Public' API key mentioned in the API docs works most of the time.

### display colors

The American EPA AQI colours used on [aqicn.org](http://aqicn.org/) are kind of horrible for blending between, already the first change from green to yellow is just way too abrupt. Nicer to use the more gradual colour scheme of the [European CAQI classes](https://en.wikipedia.org/wiki/Air_quality_index#CAQI).

### Icons

OpenWeatherMap uses 9 different icons: https://openweathermap.org/weather-conditions
But pretty difficult to find 16x16 alternatives, especially in colour?
u8g2 weather icon fonts: https://github.com/olikraus/u8g2/wiki/fntgrpiconic#open_iconic_weather_2x

https://nucleoapp.com/icons/weather

## D1 Mini shields

 | RST | TX | PMS #1 RX (HW)
free | A0 | RX | PMS #1 TX (HW)
PMS #2 TX (SW) | D0 | D1 | OLED shield SCL
SD shield CLK | D5 | D2 | OLED shield SDA
SD shield MISO | D6 | D3 | free
SD shield MOSI | D7 | D4 | SD shield CS (/`Serial1` HW TX for debug msg)
PMS #2 RX (SW) | D8 | GND |
