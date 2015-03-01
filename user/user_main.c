#include "ping/ping.h"
#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "user_config.h"
#include "user_interface.h"
#include "stdout/stdout.h"

static volatile os_timer_t loop_timer;

// forward declarations
void user_init(void);
void loop(void);
static void setup(void);
static Ping_Data pingA;
static Ping_Data pingB;

/**
 * This is the main user program loop
 */
void ICACHE_FLASH_ATTR
loop(void) {
  float distance = 0;
  float maxDistance = 2000; // 2 meter
  if (ping_ping(&pingA, maxDistance, &distance) ) {
    os_printf("A Response ~ %d mm \n", (int)distance);
  } else {
    os_printf("Failed to get any response from sensor A.\n");
  }
  if (ping_ping(&pingB, maxDistance, &distance) ) {
    os_printf("B Response ~ %d mm \n", (int)distance);
  } else {
    os_printf("Failed to get any response from sensor B.\n");
  }
}

/**
 * Setup program. When user_init runs the debug printouts will not always
 * show on the serial console. So i run the inits in here, 2 seconds later.
 */
static void ICACHE_FLASH_ATTR
setup(void) {
  ping_init(&pingA, 2, 0, PING_MM); // trigger=GPIO0, echo=GPIO2, set the pins to the same value for one-pin-mode
  ping_init(&pingB, 4, 5, PING_MM); // trigger=GPIO4, echo=GPIO5, set the pins to the same value for one-pin-mode

  // Start repeating loop timer
  os_timer_disarm(&loop_timer);
  os_timer_setfn(&loop_timer, (os_timer_func_t *) loop, NULL);
  os_timer_arm(&loop_timer, PING_SAMPLE_PERIOD, true);
}

//Init function 
void ICACHE_FLASH_ATTR
user_init(void) {
  // Make uart0 work with just the TX pin. Baud:115200,n,8,1
  // The RX pin is now free for GPIO use.
  stdout_init();

  // turn off WiFi for this console only demo
  wifi_station_set_auto_connect(false);
  wifi_station_disconnect();

  // Start setup timer
  os_timer_disarm(&loop_timer);
  os_timer_setfn(&loop_timer, (os_timer_func_t *) setup, NULL);
  os_timer_arm(&loop_timer, 2000, false);

}
