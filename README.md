# esp8266_ping
A HC-SR04, HY-SRF05 driver and example for the esp8266.
```
#include "ping/ping.h"
....
static Ping_Data pingA;
....
// setup
int triggerPin = 0;
int echoPin = 2;
ping_init(&pingA, triggerPin, echoPin, PING_MM); // set the trigger and echo pin to the same number for one-pin mode

....
// loop:
float distance = 0;
float maxDistance = 500;
if (ping_pingDistance(&pingA, maxDistance, &distance) ) {
  os_printf("Response ~ %d mm \n", (int)distance);
} else {
  os_printf("Failed to get any response.\n");
}
```

Makefile:
```
MODULES         = driver/stdout driver/easygpio driver/ping user
```

##Circuit
The HC-SR04 is a 5V device, so you will (at the very least) need a [voltage divider](http://elinux.org/RPi_GPIO_Interface_Circuits#Voltage_divider) on the echo pin.
Or even better: [a logic level shifter.](http://elinux.org/RPi_GPIO_Interface_Circuits#Level_Shifters) 

![Two pins](/doc/esp_circuit_twopins.png)
### single pin mode

![Single pin mode](/doc/esp_circuit_onepin.png)

If you want to run this in 'single pin mode' behind a logic level shifter you can connect the echo pin on the 3V3 side to the trigger GPIO via a 5KΩ resistor. This works for a [a classic MOSFET level shifter.](http://elinux.org/RPi_GPIO_Interface_Circuits#Classic_MOSFET_level_shifter) built out of two BSS138 and four 10KΩ resistors. Wiring echo & trigger together on the 5V side does *not* work with a classic mosfet level shifter.

Another working solution is a voltage divider (5V to 3.3V) between HC-SR04 echo and ground. Connect HC-SR04 trigger and esp GPIO to the middle of the divider.


### other sensors
The arduino library [newping](https://code.google.com/p/arduino-new-ping/) supports a whole range of ultrasonic sensors: SR04, SRF05, SRF06, DYP-ME007 & Parallax PING™. This without making any special hardware considerations in the code. So this library might work with those sensors as well.   

## TODO

* ~~inches~~
* ~~single pin mode~~
* ~~multiple sensors~~
* ~~sdk v0.9.5 compability~~ 
* ~~check compability SRF05~~ SRF05 works w/o any modifications
* check compability SRF06
* check compability DYP-ME007
* check compability Parallax PING™

## License

GPL V3

The makefile is copied from [esp_mqtt.](https://github.com/tuanpmt/esp_mqtt)

###Building and installing:

First you need to install the sdk and the easy way of doing that is to use [esp_open_sdk.](https://github.com/pfalcon/esp-open-sdk)

You can put that anywhere you like (/opt/local/esp-open-sdk, /esptools etc etc)

Then you could create a small ```setenv.sh``` file, containing the location of your newly compiled sdk and other platform specific info;
```
export SDK_BASE=/opt/local/esp-open-sdk/sdk
export PATH=${SDK_BASE}/../xtensa-lx106-elf/bin:${PATH}
export ESPPORT=/dev/ttyO0  
```
(or setup your IDE to do the same)

To make a clean build, flash and connect to the esp console you just do this in a shell:
```
source setenv.sh # This is only needed once per session
make clean && make test
```

You won't be needing esptool, the makefile only uses esptool.py (provided by [esp_open_sdk](https://github.com/pfalcon/esp-open-sdk))

I have tested this with sdk v1.5.2, v0.9.4 and v0.9.5 (linux & mac)
