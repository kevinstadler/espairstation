## espairstation

ESP8266/ESP32 that displays air temperature/humidity/pollution data from local and remote sensors.

Connected sensors:

* SSD1351-based 128x128px RGB OLED display 
* SR-501 PIR sensor for dynamically turning the screen on/off
* DHT22 temperature/humidity sensor for local temperature/humidity
* reads/controls a Xiaomi device over MiIo for nearby temperature/humidity data
* gathers data from the [AQICN](http://aqicn.org) JSON API for remote pollution/temperature/humidity data

The display (128x128px RGB) can choose to show any of the following data items:

* outdoor temperature (degrees) & humidity (%)
* outdoor PM2.5 particle levels (ug/m3, typical range from 0 to 200, one decimal point available)
* temperature & humidity at the device itself
* temperature & humidity at the humidifier in the room next door
* the humidity level that the humidifier is currently set to

Optional informative color coding for each of the measures (such as currently shown on the display):

* temperature: blended/mixed colours along the scale *white - blue - green - orange - red* to signify *freezing - cold - comfortable - hot*
* humidity: blended/mixed colours along a scale such as https://www.mrfixitbali.com/images/articleimages/dew-point-chart-compact.jpg to signify *dry - comfortable - muggy - horrible*
* air quality: blended/mixed colours along the [CAQI](https://en.wikipedia.org/wiki/Air_quality_index#CAQI) *green - yellow - orange - red* scale

### TODOs

* add local PM2.5 particle sensor to measure effects of any particle filters running indoors!
* improve error handling (Wifi disconnect)

### Pins/wiring

#### Available pins on the ESP8266

Pins are for a Wemos D1 Wifi:

pin(s)  | GPIO | use
------- | ---- | ---
D0      |  3 | (in use: RX)
D1      |  1 | (in use: TX)
D2      | 16 | DHT22 (sensor)
D3, D15 |  5 | PIR (detector)
D4, D14 |  4 | 
D5, D13 | 14 | SPI clock (display)
D6, D12 | 12 | display RESET
D7, D11 | 13 | SPI MOSI (display)
D8      |  0 | display DC
D9      |  2 | (in use: LED_BUILTIN)
D10     | 15 | SPI SS (display CS)
A0    | ADC0 |

#### Available pins on the ESP32

Hardware SPI (bus SPI2)

pin(s)  | GPIO | use
------- | ---- | ---
 | 14 | SPI clock (display)
 | 12 | display RESET
 | 13 | SPI MOSI (display)
SD1? / D1  |  8 | display DC
      |  2 | (in use: LED_BUILTIN)
     | 15 | SPI SS (display CS)


### SSD1351 libraries

Wasted too much time getting the display to work with various libraries:

         | ESP-32 | Wemos D1 R1 | D1 Mini | general notes
-------- | ------ | ----------- | ------- | -------------
[`ucglib`](https://github.com/olikraus/ucglib/wiki) |        | OK          | reboots during GraphicsTest | (150 stars, 54 forks) worked out of the box, has great default fonts that are easy peasy to include, great documentation. Only downside is no built-in image/bitmap support?
`lcdgfx` (and `ssd1306`) |        | works | skips 4 pixel lines in the middle of the display | works, super customizable but font selection/support seemed too cumbersome to get started quickly, doxygen docs are a bit of a pain to navigate/understand
Adafruit `ssd1351` | | artefacts | artefacts | skips every second line on the ESP8266 using both hardware and software SPI. There are some strange `#ifdef` clauses in the library that bypass some constructors for the ESP8266, so gave up on it...
[FTOLED](https://github.com/freetronics/FTOLED/wiki/Function-Reference) | | |
[`OLED_eSPI`])https://github.com/MagicCube/OLED_eSPI) | | | not working | fork of TFT_eSPI

Next steps/salvaging: because the code (or rather the display) I worked on for the Uno-sized D1 does not port to the D1 mini I need to pursue one of two other options:

1. use a SSD1331-based 64px display instead (on the D1 mini)
2. use the same display but on a ESP32 board
3. run on one of the NodeMCUs instead (what was wrong with them again?)

### SR-501 PIR sensor

Setting the jumper to "repeat trigger" and the delay quite high (15+ seconds) allows you to control the screen (powerUp/powerDown) via a `CHANGE` interrupt on the data pin. The sensor takes about a minute to adjust to baseline lighting, so only attach the interrupt at the end of the `setup()` return after waiting for about a minute.

### DHT22 temperature/humidity sensor

Is constantly reading data when it's only required to update every 5 minutes or so, maybe possible to supply voltage via one of the digital Pins?

TODO: need to figure out consumption of the DHT22 and the maximum current draw for the digital pins.

* https://forum.arduino.cc/index.php?topic=332915.0
* https://forum.mysensors.org/topic/323/powering-sensors-through-the-digital-pins/3

### MiIo / how to get your MiIO token

<!-- deerma-humidifier-mjjsq_miapEB50 -->

My device model is a Xiaomi Humidifier 2(?) aka `deerma.humidifier.mjjsq`. For access I used ar2rus' experimental [ESPMiIO](https://github.com/ar2rus/ESPMiIO/) library with some modifications (see my [fork](https://github.com/kevinstadler/ESPMiIO/)).

For obtaining the MiIo token required to connect to the device I followed https://github.com/Maxmudjon/com.xiaomi-miio/blob/master/docs/obtain_token.md but none of the elegant methods worked: Arduino backup was a pain, [even with setting a backup desktop password](https://android.stackexchange.com/questions/116439/adb-backup-command-on-non-rooted-device-creates-an-empty-backup-file) it just didn't work, the auto-token obtained via the `miio` command line tool would also expire after connecting the device to Wifi, so ended up installing the verbosely logging Xiaomi Home version instead.

### AQICN JSON API

See [hackster.io: ESP8266 AQI Display](https://www.hackster.io/arkhan/esp8266-aqi-display-25bba7)

### Scheduling updates

* humidifier status every 10 minutes?
* local sensor every 10 minutes?
* AQI every 10 minutes?

TODO which library/timer to use to schedule updates?

* expiration-based: https://circuits4you.com/2018/01/02/esp8266-timer-ticker-example/

ntp/time-based

* https://lastminuteengineers.com/esp8266-ntp-server-date-time-tutorial/
* https://forum.arduino.cc/index.php?topic=608549.0
* Uses https://github.com/jhagberg/ESP8266TimeAlarms

### PM2.5 sensors

All laser-based, 5V power, 3.3V logic level devices communicating via Serial.

* [Plantower PMS 7003](https://www.espruino.com/datasheets/PMS7003.pdf) +-10ug accuracy, 100mA usage, .2mA standby, has a sleep pin but does not provide 3.3V out. ([Arduino driver],(https://www.espruino.com/PMS7003), [AQICN review](https://aqicn.org/sensor/pms5003-7003/), [Plantower product website](http://www.plantower.com/list/?6_1.html): 7003M model has fan+exhaust at the top, normal/P/I modals 90 opposed on the side?)
* [Honeywell HPMA115SO-XXX](https://sensing.honeywell.com/honeywell-sensing-particulate-hpm-series-datasheet-32322550) +-15ug accuracy, 600mA surge voltage, 80mA usage, 20mA standby, has a 3.3V 100mA output pin.... ([Arduino driver](https://github.com/felixgalindo/HPMA115S0))

Laboratory experiment comparisons:

* https://uwspace.uwaterloo.ca/bitstream/handle/10012/12776/Tan_Ben.pdf?sequence=5&isAllowed=y

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

## PMS7003 libraries

PMSSerial (`PMSerial.h`) takes a Serial object for HW and has SoftwareSerial 'built-in', is always in pass has an OLED example sketch and is also nicely blocking.

PMS Library (`PMS.h`) takes a Serial object as argument and has methods for turning on/off passive mode but does not block...
