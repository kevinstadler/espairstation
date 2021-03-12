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
[`ucglib`](https://github.com/olikraus/ucglib/wiki) | | OK          | reboots during GraphicsTest | (150 stars, 54 forks) worked out of the box, has great default fonts that are easy peasy to include, great documentation. Only downside is no built-in image/bitmap support?
`lcdgfx` (and `ssd1306`) |        | works | skips 4 pixel lines in the middle of the display | works, super customizable but font selection/support seemed too cumbersome to get started quickly, doxygen docs are a bit of a pain to navigate/understand
Adafruit `ssd1351` | | artefacts | artefacts | skips every second line on the ESP8266 using both hardware and software SPI. There are some strange `#ifdef` clauses in the library that bypass some constructors for the ESP8266, so gave up on it...
[FTOLED](https://github.com/freetronics/FTOLED/wiki/Function-Reference) | | |
[`OLED_eSPI`])https://github.com/MagicCube/OLED_eSPI) | | | not working | fork of TFT_eSPI

Next steps/salvaging: because the code (or rather the display) I worked on for the Uno-sized D1 does not port to the D1 mini I need to pursue one of two other options:

1. use a SSD1331-based 64px display instead (on the D1 mini)
2. use the same display but on a ESP32 board

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
