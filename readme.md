Rewrite [AButton](http://wiki.aprbrother.com/wiki/AButton) with Arduino

### How To

#### With PlatformIO

* Install [platformio](https://github.com/esp8266/arduino#using-platformio)
* Run command ```make flash``` to upload firmware

#### With Arduino

* Install ESP8266 Arduino core
* Install [libraries](#libraries-dependency) with library manager
* Open src.ino and compile

### LED Status

* Red - AP mode for configure
* Blue - WiFi connected
* Green - Sent request to custom url

### Libraries Dependency

* Adafruit_NeoPixel
* ArduinoJson
* WiFiManager

See the comments in [platformio.ini](platformio.ini)

### OTA Upgrade

* Keep long press button
* Wait for green LED link -> red LED blink
* Access uri /update to upload new firmware
