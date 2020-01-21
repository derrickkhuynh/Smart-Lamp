# SmartLampDisplay
Created a pair of smart lamps that connect to the Wifi and has additional features when both lamps are on. 
Includes display option that displays time, weather and custom message.

Utilizes an ESP32 MCU, a neopixel ring, TFT Display, and a capactive sensor of your choice.

There are 3 different version of the code:
1. NoDisplay_TouchSensor.cpp  -> only lamp portion, includes a sparkfun capacitive touch sensor.
2. NoDisplay_TouchLibrary.cpp -> only lamp portion, uses the CapacitiveSensor library.
3. Display_TouchSensor.cpp    -> lamp +TFT display, includes a sparkfun capacitive touch sensor.

Display_TouchSensor.cpp is still a work in progress and the text locations, colors, etc. are still being edited
You are free to change the display as you like.

Coded in PlatformIO IDE using arduino framework.
