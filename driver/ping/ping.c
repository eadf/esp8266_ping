/*
* ping.c
*
* Copyright (c) 2015, eadf (https://github.com/eadf)
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
* * Redistributions of source code must retain the above copyright notice,
* this list of conditions and the following disclaimer.
* * Redistributions in binary form must reproduce the above copyright
* notice, this list of conditions and the following disclaimer in the
* documentation and/or other materials provided with the distribution.
* * Neither the name of Redis nor the names of its contributors may be used
* to endorse or promote products derived from this software without
* specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*/
#include "ping/ping.h"
#include "osapi.h"
#include "ets_sys.h"
#include "gpio.h"
#include "mem.h"
#include "easygpio/easygpio.h"
#include "os_type.h"

#define ping_micros (0x7FFFFFFF & system_get_time())
#define PING_TRIGGER_DEFAULT_STATE 0
#define PING_TRIGGER_LENGTH 5
#define PING_POLL_PERIOD 100 // 100 us, used when polling interrupt results

static volatile uint32_t   ping_timeStamp0 = 0;
static volatile bool       ping_echoStarted = false;
static volatile uint32_t   ping_timeStamp1 = 0;
static volatile bool       ping_echoEnded = false;

static uint8_t ping_triggerPin = 255;
static uint8_t ping_echoPin = 255;
static bool singlePinMode = false;

static bool ping_isInitiated = false;

// forward declarations
static void ping_disableInterrupt(void);
static void ping_intr_handler(void);


static void
ping_disableInterrupt(void) {
  gpio_pin_intr_state_set(GPIO_ID_PIN(ping_echoPin), GPIO_PIN_INTR_DISABLE);
}

static void
ping_intr_handler(void) {
  uint32_t gpio_status = GPIO_REG_READ(GPIO_STATUS_ADDRESS);
  //clear interrupt status
  GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, gpio_status & BIT(ping_echoPin));

  if(!ping_echoStarted) {
    gpio_pin_intr_state_set(GPIO_ID_PIN(ping_echoPin), GPIO_PIN_INTR_NEGEDGE);
    ping_timeStamp0 = ping_micros;
    ping_echoStarted = true;
  } else {
    ping_timeStamp1 = ping_micros;
    ping_echoEnded = true;
    ping_disableInterrupt();
  }
}

/**
 * Sends a ping, and returns the number of microseconds it took to receive a response.
 * Will give up after maxPeriod (with false as return value)
 */
bool ICACHE_FLASH_ATTR
ping_ping(uint32_t maxPeriod, uint32_t* response) {
  uint32_t timeOutAt = ping_micros + maxPeriod;
  ping_echoEnded = false;
  ping_echoStarted = false;
  ping_timeStamp0 = ping_micros;

  if (!ping_isInitiated) {
    *response = 0;
    os_printf("ping_ping: Error: not initiated properly.\n");
    return false;
  }
  if (singlePinMode){
    GPIO_DIS_OUTPUT(ping_echoPin);
    os_delay_us(10);
  }
  while (GPIO_INPUT_GET(ping_echoPin)) {
    if (ping_micros > timeOutAt) {
      // echo pin never went low, something is wrong.

      // turns out this happens whenever the sensor doesn't receive any echo at all.

      os_printf("ping_ping: Error: echo pin %d permanently high %d?.\n", ping_echoPin, GPIO_INPUT_GET(ping_echoPin));
      *response = ping_micros - ping_timeStamp0;

      // Wake up a sleeping device
      GPIO_OUTPUT_SET(ping_triggerPin, PING_TRIGGER_DEFAULT_STATE);
      os_delay_us(50);
      GPIO_OUTPUT_SET(ping_triggerPin, !PING_TRIGGER_DEFAULT_STATE);
      os_delay_us(50);
      GPIO_OUTPUT_SET(ping_triggerPin, PING_TRIGGER_DEFAULT_STATE);

      return false;
    }
    os_delay_us(PING_POLL_PERIOD);
  }


  if (singlePinMode){
    GPIO_OUTPUT_SET(ping_triggerPin, 1);
    os_delay_us(PING_TRIGGER_LENGTH);
    GPIO_OUTPUT_SET(ping_triggerPin, 0);
  } else {
    GPIO_OUTPUT_SET(ping_triggerPin, !PING_TRIGGER_DEFAULT_STATE);
    os_delay_us(PING_TRIGGER_LENGTH);
    GPIO_OUTPUT_SET(ping_triggerPin, PING_TRIGGER_DEFAULT_STATE);
  }
  GPIO_DIS_OUTPUT(ping_echoPin);
  gpio_pin_intr_state_set(GPIO_ID_PIN(ping_echoPin), GPIO_PIN_INTR_POSEDGE);

  while (!ping_echoEnded) {
    if (ping_micros > timeOutAt) {
      *response = ping_micros - ping_timeStamp0;
      return false;
    }
    os_delay_us(PING_POLL_PERIOD);
  }

  *response = ping_timeStamp1 - ping_timeStamp0;
  if (ping_timeStamp1 < ping_timeStamp0 || *response < 50) {
    // probably a previous echo or clock overflow - false result
    return false;
  }
  return true;
}

bool ICACHE_FLASH_ATTR
ping_pingDistance(Ping_Unit unit, float maxDistance, float* returnDistance) {
  uint32_t echoTime = 0;
  uint32_t maxPeriod = 0;

  switch (unit) {
    case PING_MM:
      maxPeriod = maxDistance/PING_US_TO_MM;
      break;
    case PING_INCHES:
      maxPeriod = maxDistance/PING_US_TO_INCH;
      break;
    default:
      os_printf("ping_pingDistance: Error: unit=%d is undefined\n", unit );
      return false;
  }
  if (!ping_ping(maxPeriod, &echoTime)) {
    //os_printf("ping_ping failed: maxPeriod=%d echoTime=%d\n",maxPeriod, (int)echoTime );
    return false;
  }
  switch (unit) {
    case PING_MM:
      *returnDistance = ((float)echoTime)*PING_US_TO_MM;
      break;
    case PING_INCHES:
      *returnDistance = ((float)echoTime)*PING_US_TO_INCH;
      break;
    default:
      os_printf("ping_pingDistance: Error: unit=%d is undefined\n", unit );
      return false;
  }
  return true;
}

/**
 * Initiates the GPIOs
 */
void ICACHE_FLASH_ATTR
ping_init(uint8_t triggerPin, uint8_t echoPin) {
  ping_triggerPin = triggerPin;
  ping_echoPin = echoPin;

  if (ping_triggerPin == ping_echoPin) {
    singlePinMode = true;

  } else {

    if (!easygpio_pinMode(ping_triggerPin, EASYGPIO_NOPULL, EASYGPIO_OUTPUT)){
      os_printf("ping_init: Error: failed to set pinMode on trigger pin\n");
      return;
    }
    GPIO_OUTPUT_SET(ping_triggerPin, PING_TRIGGER_DEFAULT_STATE);
  }

  if (easygpio_attachInterrupt(ping_echoPin, EASYGPIO_NOPULL, ping_intr_handler)) {
    ping_disableInterrupt();
    if (singlePinMode) {
      GPIO_OUTPUT_SET(ping_triggerPin, 0);  // 0 is default output when single pin mode
    }
    os_printf("\nInitiated ping module with trigger pin = %d echo pin = %d. singlepinmode=%d\n\n",ping_triggerPin, ping_echoPin, singlePinMode);
    ping_isInitiated = true;
  } else {
    os_printf("ping_init: Error: failed to set interrupt on echo pin %d\n", ping_echoPin);
    ping_isInitiated = false;
  }

}
