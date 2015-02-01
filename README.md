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
uint32_t period = 0;
uint32_t maxPeriod = 5000;
if (ping_ping(maxPeriod, &period) ) {
  os_printf("Response %d us ~ %d mm \n", period, (int)(((float)period)/5.8));
} else {
  os_printf("Failed to get any response within %d us period\n", period);
}
```

Makefile:
```
MODULES         = driver easygpio ping user
```

##Required:

esp_iot_sdk_v0.9.4_14_12_19 ( v0.9.5 breaks everything ) 


## TODO

* inches
* single pin mode
* sdk v0.9.5 compability

## License

GPL V3
