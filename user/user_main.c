#include "ping/ping.h"
#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "user_config.h"
#include "user_interface.h"
#include "driver/stdout.h"

#define user_procTaskPrio        0
#define user_procTaskQueueLen    1
static volatile os_timer_t loop_timer;
os_event_t user_procTaskQueue[user_procTaskQueueLen];

// forward declarations
static void nop_procTask(os_event_t *events);
void user_init(void);
void loop(void);
static void setup(void);

/**
 * This is the main user program loop
 */
void ICACHE_FLASH_ATTR
loop(void) {
  static uint8_t iterations = 0;
  uint32_t period = 0;
  if (ping_ping(5000, &period) ) {
    os_printf("Response %d us ~ %d mm \n", period, (int)(((float)period)/5.8));
  } else {
    os_printf("Failed to get any response within %d us period\n", period);
  }
  iterations += 1;
}

/**
 * Setup program. When user_init runs the debug printouts will not always
 * show on the serial console. So i run the inits in here, 2 seconds later.
 */
static void ICACHE_FLASH_ATTR
setup(void) {
  ping_init(0,2);
  // Start loop timer
  os_timer_disarm(&loop_timer);
  os_timer_setfn(&loop_timer, (os_timer_func_t *) loop, NULL);
  os_timer_arm(&loop_timer, 500, 1);
}

//Do nothing function
static void ICACHE_FLASH_ATTR
nop_procTask(os_event_t *events) {
  os_delay_us(10);
}

//Init function 
void ICACHE_FLASH_ATTR
user_init(void) {
  stdoutInit();

  //Set station mode
  wifi_set_opmode( NULL_MODE );

  // Start setup timer
  os_timer_disarm(&loop_timer);
  os_timer_setfn(&loop_timer, (os_timer_func_t *) setup, NULL);
  os_timer_arm(&loop_timer, 2000, 0);

  //Start no-operation os task
  system_os_task(nop_procTask, user_procTaskPrio, user_procTaskQueue, user_procTaskQueueLen);
  system_os_post(user_procTaskPrio, 0, 0);
}
