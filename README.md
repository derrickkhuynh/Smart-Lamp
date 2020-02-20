# Love Lamp
Created a pair of smart lamps that connect to the Wifi and has additional features when both lamps are on. 
Includes display option that displays time, weather and custom message.

Utilizes an ESP32 MCU, a neopixel ring, TFT Display, and a capactive sensor of your choice.

There are 2 different version of the code:
1. noDisplay.cpp -> only lamp portion
2. display.cpp   -> lamp +TFT display

## Features:
Simple touch on/off, unlimited color customization, provides current time, date and weather, customized message sending.

## Bill of Materials
https://docs.google.com/spreadsheets/d/1QPAhnyyFz2ygeMsZnL3zneTPxA3fL7MyY0FP0qiF18g/edit?usp=sharing

## Credit to:
reddit user u/ralphkrs and his code for main idea of project at 
https://www.reddit.com/r/arduino/comments/eftbnd/happy_holidays_kind_of_a_newbie_with_arduino_and/
https://gist.github.com/rlphkrs/25a04761eda68924b2303f3cd747828e

Idea of combining with current time from Sudomod at
https://sudomod.com/wifi-neopixels-esp32/

Idea of using NTP server and MQTT Server
https://www.hackster.io/AskSensors/publish-esp32-data-to-asksensors-with-timestamp-over-mqtt-792d09 

cloudMQTT for providing MQTT Server
https://cloudmqtt.com

Pool.ntp.org for NTP Server
https://www.ntppool.org/en/use.html

OpenWeatherMap for API usage for weather info
https://openweathermap.org/ 

Adafruit for providing resources on using the Neopixel Ring
https://cdn-learn.adafruit.com/downloads/pdf/adafruit-neopixel-uberguide.pdf

Makuna for providing NeoPixelBus library
https://github.com/Makuna/NeoPixelBus/wiki/NeoPixelBus-object-API 

Khoih-prog for providing ESP_WiFiManager library
https://github.com/khoih-prog/ESP_WiFiManager 

Bodmer for providing TFT_eSPI Library and resources on forums for troubleshooting
https://github.com/Bodmer/TFT_eSPI 

Entire program was created using PlatformIO IDE on VS Code. 
https://platformio.org/platformio-ide 
