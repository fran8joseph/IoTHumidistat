# IoTHumidistat
A Humidistat that throws information on a HTML page in a local network via WiFi 

IoT - NodeMcu using DHT11 &amp; Servo Motor

Upgraded my previous implementation of the Humidistat using Arduino Uno (https://github.com/francis-learningtech/humidiStat) which was working flawlessly, but still it required manual intervention for initial set up plus displaying current time was not possible without a RTC module. 

So I thought of upgrading it to next level, by addind WiFi feature to the existing board. Once I got NodeMcu ESP8266 and started exploring it, I was taken by surprise knowing the capability of this tiny module. It is not just a WiFi extension, it is a complete Microcontroller itself. 
There is a ton of information on how to use it, is available in the internet and I have gotten very good guidance by following some of them which I have listed below. 

## Functionality:
The NodeMcu module requests current Weather info from [OpenWeather](https://openweathermap.org/) for a given location. _OpenWeather has free API service which anyone can use to get basic weather information and I have used the same_.

The NodeMCU module gathers room Temperature and Humidity data from DTH11 sensor and a logic inside the Microcontroller, compares outdoor temperature value from the _OpenWeather_ API and it will then decide the optimum *Humidity* value to set inside. 
If the existing humidity inside the room is less than the _optimum_ value, it switches ON humidifier via a _Micro Servo_ motor, which turns the knob and then the humidifier is ON. 

NodeMCU also acts a web server, which can be used to display information on a local network. _I haven't explored sending info over the internet yet_ 
The Microcontroller repeats the above steps every minute and displays that in a HTML page in local network, which can be checked within the WiFi network. 

### Reference 
1. Weather Info - https://openweathermap.org/
2. Current time - http://worldclockapi.com/
3. ESP8266
  1. Installation - https://forum.arduino.cc/index.php?topic=649739.msg4401959#msg4401959
  2. https://stackoverflow.com/questions/50080260/arduino-ide-cant-find-esp8266wifi-h-file
  3. Server info: 
        https://tttapa.github.io/ESP8266/Chap10%20-%20Simple%20Web%20Server.html
  4. Architecture 
        https://arduino-esp8266.readthedocs.io/en/latest/libraries.html
        https://randomnerdtutorials.com/esp8266-web-server/        
        https://www.instructables.com/id/Interfacing-Servo-Motor-With-NodeMCU/
        https://randomnerdtutorials.com/esp8266-pinout-reference-gpios/       
        https://circuits4you.com/2018/12/31/esp32-pwm-example/
        https://components101.com/servo-motor-basics-pinout-datasheet
        https://tttapa.github.io/ESP8266/Chap04%20-%20Microcontroller.html
        https://www.instructables.com/id/Arduino-IOT-Temperature-and-Humidity-With-ESP8266-/
5. Humidity range
    https://www.aprilaire.com/docs/default-source/default-document-library/relative-humidity-defined.pdf?sfvrsn=2
    https://www.hvac.com/faq/recommended-humidity-level-home/     
