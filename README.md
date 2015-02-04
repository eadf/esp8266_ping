# esp8266_ping
The initial stages of a HC-SR04 library for the esp8266.
```
#include "ping/ping.h"
....
// setup
int triggerPin = 0;
int echoPin = 2;
ping_init(triggerPin,echoPin);
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

If you want to run this in 'single pin mode' behind a logic level shifter you can connect the echo pin on the 3V3 side to a 1Kohm resistor and then to the trigger pin. That works for me at least. 

Wiring echo & trigger together on the 5V side does *not* work.

The arduino library [newping](https://code.google.com/p/arduino-new-ping/) supports a whole range of ultrasonic sensors: SR04, SRF05, SRF06, DYP-ME007 & Parallax PING™. This without making any special hardware considerations in the code. So this library might work with those sensors as well.   

##Required:

esp_iot_sdk_v0.9.4_14_12_19 ( v0.9.5 breaks everything ) 

Actually, i have not tested this with v0.9.5. I tested a [clean sdk 0.9.5 install](https://github.com/pfalcon/esp-open-sdk) with one of the basic examples (could have been blinky). It compiled and uploaded fine but the esp had a infinite crash loop with some message about "MEM CHK FAIL" on the console. So i threw the whole sdk out (aint nobody got time fo dat). I will try upgrading the sdk again once [mqtt](https://github.com/tuanpmt/esp_mqtt) upgrades to 0.9.5+.

## TODO

* ~~inches~~
* ~~single pin mode~~
* multiple sensors at once
* sdk v0.9.5 compability 

## License

GPL V3
