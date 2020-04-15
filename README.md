## espairstation

ESP8266/ESP32 that displays air temperature/humidity/pollution data from local and remote sensors.

Connected sensors:

* SSD1351-based 128x128px RGB OLED display 
* SR-501 PIR sensor for dynamically turning the screen on/off
* DHT22 temperature/humidity sensor for local temperature/humidity
* reads/controls a Xiaomi device over MiIo for nearby temperature/humidity data
* gathers data from the [AQICN](http://aqicn.org) JSON API for remote pollution/temperature/humidity data

### Pins/wiring

Available pins on the ESP8266:

pin(s)  | GPIO | use
------- | ---- | ---
D0      |  1 | (in use: TX)
D1      |  3 | (in use: RX)
D2      | 16 | DHT22 (sensor)
D3, D15 |  5 | PIR (detector)
D4, D14 |  4 | 
D5, D13 | 14 | SPI clock (display)
D6, D12 | 12 | display RESET
D7, D11 | 13 | SPI MOSI (display)
D8      |  0 | display DC
D9      |  2 | (in use: LED_BUILTIN)
D10     | 15 | display CS

### SSD1351 libraries

Wasted too much time getting the display to work with various libraries:

* eventually settled on [ucglib](https://github.com/olikraus/ucglib/wiki) (150 stars, 54 forks) because it worked out of the box, and has great default fonts that are easy peasy to include, great documentation. Only downside is no image/bitmap support?
* `lcdgfx` works, super customizable but font selection/support seemed too cumbersome to get started quickly, doxygen docs are a bit of a pain to navigate/understand
* adafruit `libglx`/ssd1351 has great printing functions but stupid issue with ESP8266, skips every second line or so!?? Tried both hardware and software SPI, there are some strange `#ifdef` clauses in the library that bypass some constructors for the ESP8266, so gave up on it...
* Another potential option I never actually tried out: [FTOLED](https://github.com/freetronics/FTOLED/wiki/Function-Reference) (27 stars, 15 forks)

### MiIo / how to get your MiIO token

My device model is a Xiaomi Humidifier 2(?) aka `deerma.humidifier.mjjsq`. For access I used ar2rus' experimental [ESPMiIO](https://github.com/ar2rus/ESPMiIO/) library with some modifications (see my [fork](https://github.com/kevinstadler/ESPMiIO/)).

For obtaining the MiIo token required to connect to the device I followed https://github.com/Maxmudjon/com.xiaomi-miio/blob/master/docs/obtain_token.md but none of the elegant methods worked: Arduino backup was a pain, [even with setting a backup desktop password](https://android.stackexchange.com/questions/116439/adb-backup-command-on-non-rooted-device-creates-an-empty-backup-file) it just didn't work, the auto-token obtained via the `miio` command line tool would also expire after connecting the device to Wifi, so ended up installing the verbosely logging Xiaomi Home version instead.

### Scheduling updates

* humidifier status every 10 minutes?
* local sensor every 10 minutes?
* AQI every 10 minutes?

TODO which library/timer to use to schedule updates?

* https://github.com/jhagberg/ESP8266TimeAlarms
