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
MODULES         = driver easygpio ping user
```

##Circuit
The HC-SR04 is a 5V device, so you will (at the very least) need a [voltage divider](http://elinux.org/RPi_GPIO_Interface_Circuits#Voltage_divider) on the echo pin.
Or even better: [logic level shifter](http://elinux.org/RPi_GPIO_Interface_Circuits#Level_Shifters) 

##Required:

esp_iot_sdk_v0.9.4_14_12_19 ( v0.9.5 breaks everything ) 

Actually, i have not tested this with v0.9.5. I tested a clean sdk 0.9.5 install with one of the basic examples (could have been blinky). It compiled and uploaded fine but the esp had a infinite crash loop with some message about "MEM CHK FAIL" on the console. So i threw the whole sdk out in disgust. I will try 0.9.5 again once [mqtt](https://github.com/tuanpmt/esp_mqtt) upgrades.

## TODO

* ~~inches~~
* single pin mode
* sdk v0.9.5 compability

## License

GPL V3
