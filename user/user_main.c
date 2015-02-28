#include "ping/ping.h"
#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "user_config.h"
#include "user_interface.h"
#include "stdout/stdout.h"

#define user_procTaskPrio        0
#define user_procTaskQueueLen    1
static volatile os_timer_t loop_timer;
os_event_t user_procTaskQueue[user_procTaskQueueLen];

// forward declarations
void user_init(void);
void loop(void);
static void setup(void);

/**
 * This is the main user program loop
 */
void ICACHE_FLASH_ATTR
loop(void) {
  float distance = 0;
  float maxDistance = 1000; // 1 meter
  if (ping_pingDistance(PING_MM, maxDistance, &distance) ) {
    os_printf("Response ~ %d mm \n", (int)distance);
  } else {
    os_printf("Failed to get any response.\n");
  }
}

/**
 * Setup program. When user_init runs the debug printouts will not always
 * show on the serial console. So i run the inits in here, 2 seconds later.
 */
static void ICACHE_FLASH_ATTR
setup(void) {
  uint8_t echoPin = 2;
  uint8_t triggerPin = 0;
  ping_init(triggerPin, echoPin); // trigger=GPIO0, echo=GPIO2, set the pins to the same value for one-pin-mode
  // Start repeating loop timer
  os_timer_disarm(&loop_timer);
  os_timer_setfn(&loop_timer, (os_timer_func_t *) loop, NULL);
  os_timer_arm(&loop_timer, PING_SAMPLE_PERIOD, true);
}

//Init function 
void ICACHE_FLASH_ATTR
user_init(void) {
  stdout_init();

  // turn off WiFi for this console only demo
  wifi_station_set_auto_connect(false);
  wifi_station_disconnect();

  // Start setup timer
  os_timer_disarm(&loop_timer);
  os_timer_setfn(&loop_timer, (os_timer_func_t *) setup, NULL);
  os_timer_arm(&loop_timer, 2000, false);

}
