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
The HC-SR04 is a 5V device, so you will (at the very least) need a 
[logic level shifter](http://elinux.org/RPi_GPIO_Interface_Circuits) on the echo pin.

##Required:

esp_iot_sdk_v0.9.4_14_12_19 ( v0.9.5 breaks everything ) 


## TODO

* ~~inches~~
* single pin mode
* sdk v0.9.5 compability

## License

GPL V3
