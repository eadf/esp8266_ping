# esp8266_ping
The initial stages of a HC-SR04 library for the esp8266.
```
#include "ping/ping.h"
....
// setup
int triggerPin = 0;
int echoPin = 2;
ping_init(triggerPin,echoPin); // set the trigger and echo pin to the same number for one-pin mode
....
// loop:
float distance = 0;
float maxDistance = 500;
if (ping_pingDistance(PING_MM, maxDistance, &distance) ) {
  os_printf("Response ~ %d mm \n", (int)distance);
} else {
  os_printf("Failed to get any response.\n");
}
```

Makefile:
```
MODULES         = driver driver/easygpio driver/ping user
```

##Circuit
The HC-SR04 is a 5V device, so you will (at the very least) need a [voltage divider](http://elinux.org/RPi_GPIO_Interface_Circuits#Voltage_divider) on the echo pin.
Or even better: [logic level shifter](http://elinux.org/RPi_GPIO_Interface_Circuits#Level_Shifters) 

### single pin mode
If you want to run this in 'single pin mode' behind a logic level shifter you can connect the echo pin on the 3V3 side to the trigger GPIO via a 5KΩ resistor. That works for me at least. 

esp8266 | via | level shifter 3V3| level shifter 5V | HC-SR04
--------|-----|-------------|-----------------------|------------
GPIOX   | -   | port1 3V3   | port1 5V| trigger
GPIOX   | 5kΩ   | port2 3V3   | port2 5V| echo

It would be nice if some EE could verify this hookup.

Wiring echo & trigger together on the 5V side does *not* work.


### other sensors
The arduino library [newping](https://code.google.com/p/arduino-new-ping/) supports a whole range of ultrasonic sensors: SR04, SRF05, SRF06, DYP-ME007 & Parallax PING™. This without making any special hardware considerations in the code. So this library might work with those sensors as well.   

## TODO

* ~~inches~~
* ~~single pin mode~~
* multiple sensors at once
* ~~sdk v0.9.5 compability~~ 

## License

GPL V3

###Building and installing:

First you need to install the sdk and the easy way of doing that is to use [esp_open_sdk.](https://github.com/pfalcon/esp-open-sdk)

You can put that anywhere you like (/opt/local/esp-open-sdk, /esptools etc etc)

Then you could create a small setenv.sh file, containing the location of your newly compiled sdk and other platform specific info;
```
export SDK_BASE=/opt/local/esp-open-sdk/sdk
export PATH=${SDK_BASE}/../xtensa-lx106-elf/bin:${PATH}
export ESPPORT=/dev/ttyO0  
```
(or setup your IDE to do the same)

In the root of this project create a soft link Makefile -> Makefile.[mac,linux]
```
ln -s Makefile.linux Makefile
```
You don't *have* to do this, it just makes it more convenient to run ```make``` (instead of ```make -f Makefile.linux```)

To make a clean build, flash and connect to the esp console you just do this in a shell:
```
source setenv.sh # This is only needed once per session
make clean && make test
```

You won't be needing esptool, my makefiles only uses esptool.py (provided by esp-open-sdk)

I have tested this with sdk v0.9.5 and v0.9.4 (linux & mac makefile)
