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

#define PING_TRIGGER_DEFAULT_STATE 0
#define PING_TRIGGER_LENGTH 5
#define PING_POLL_PERIOD 100 // 100 us, used when polling interrupt results

static volatile uint32_t   ping_timeStamp0 = 0;
static volatile bool       ping_echoStarted = false;
static volatile uint32_t   ping_timeStamp1 = 0;
static volatile bool       ping_echoEnded = false;
static volatile int8_t     ping_currentEchoPin = -1;
static volatile uint32_t   ping_allEchoPins = 0; // a mask containing all of the initiated interrupt pins

// forward declarations
static void ping_disableInterrupt(int8_t pin);
static void ping_intr_handler(int8_t key);


static void
ping_disableInterrupt(int8_t pin) {
  if (pin>0){
    gpio_pin_intr_state_set(GPIO_ID_PIN(pin), GPIO_PIN_INTR_DISABLE);
  }
}

static void
ping_intr_handler(int8_t key) {
  uint32_t gpio_status = GPIO_REG_READ(GPIO_STATUS_ADDRESS);
  if (ping_allEchoPins & gpio_status) {
    if (ping_currentEchoPin>=0 && (gpio_status & BIT(ping_currentEchoPin))) {
      // This interrupt was intended for us - clear interrupt status
      GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, gpio_status & BIT(ping_currentEchoPin));

      if(!ping_echoStarted) {
        gpio_pin_intr_state_set(GPIO_ID_PIN(ping_currentEchoPin), GPIO_PIN_INTR_NEGEDGE);
        ping_timeStamp0 = system_get_time();
        ping_echoStarted = true;
      } else {
        ping_timeStamp1 = system_get_time();
        ping_echoEnded = true;
        ping_disableInterrupt(ping_currentEchoPin);
        ping_currentEchoPin = -1;
      }
    } else {
      // This interrupt was intended for us, but not at this moment - clear interrupt status anyway
      GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, gpio_status & ping_allEchoPins);
    }
  }
}

/**
 * Sends a ping, and returns the number of microseconds it took to receive a response.
 * Will give up after maxPeriod (with false as return value)
 */
bool ICACHE_FLASH_ATTR
ping_pingUs(Ping_Data *pingData, uint32_t maxPeriod, uint32_t* response) {
  uint32_t timeOutAt = system_get_time() + maxPeriod;
  ping_echoEnded = false;
  ping_echoStarted = false;
  ping_timeStamp0 = system_get_time();

  if (!pingData->isInitiated) {
    *response = 0;
    os_printf("ping_pingUs: Error: not initiated properly.\n");
    return false;
  }
  if (ping_currentEchoPin >= 0) {
    // this should not really happend, how did you end up here?
    *response = 0;
    os_printf("ping_pingUs: Error: another ping is already running.\n");
    return false;
  }

  uint32_t echoPin = pingData->echoPin;
  uint32_t triggerPin = pingData->triggerPin;
  ping_currentEchoPin = pingData->echoPin;

  while (GPIO_INPUT_GET(echoPin)) {
    if (system_get_time() > timeOutAt) {
      // echo pin never went low, something is wrong.
      // turns out this happens whenever the sensor doesn't receive any echo at all.

      //os_printf("ping_ping: Error: echo pin %d permanently high %d?.\n", echoPin, GPIO_INPUT_GET(echoPin));
      *response = system_get_time() - ping_timeStamp0;

      // Wake up a sleeping device
      GPIO_OUTPUT_SET(triggerPin, PING_TRIGGER_DEFAULT_STATE);
      os_delay_us(50);
      GPIO_OUTPUT_SET(triggerPin, !PING_TRIGGER_DEFAULT_STATE);
      os_delay_us(50);
      GPIO_OUTPUT_SET(triggerPin, PING_TRIGGER_DEFAULT_STATE);
      ping_disableInterrupt(ping_currentEchoPin);
      ping_currentEchoPin = -1;
      return false;
    }
    os_delay_us(PING_POLL_PERIOD);
  }

  GPIO_OUTPUT_SET(triggerPin, 1);
  os_delay_us(PING_TRIGGER_LENGTH);
  GPIO_OUTPUT_SET(triggerPin, 0);

  GPIO_DIS_OUTPUT(echoPin);
  gpio_pin_intr_state_set(GPIO_ID_PIN(echoPin), GPIO_PIN_INTR_POSEDGE);

  while (!ping_echoEnded) {
    if (system_get_time() > timeOutAt) {
      *response = system_get_time() - ping_timeStamp0;
      ping_disableInterrupt(ping_currentEchoPin);
      ping_currentEchoPin = -1;
      return false;
    }
    os_delay_us(PING_POLL_PERIOD);
  }

  *response = ping_timeStamp1 - ping_timeStamp0;
  if (ping_timeStamp1 < ping_timeStamp0 || *response < 50) {
    // probably a previous echo or clock overflow - false result
    ping_disableInterrupt(ping_currentEchoPin);
    ping_currentEchoPin = -1;
    return false;
  }
  return true;
}

bool ICACHE_FLASH_ATTR
ping_ping(Ping_Data *pingData, float maxDistance, float* returnDistance) {
  uint32_t echoTime = 0;
  uint32_t maxPeriod = 0;

  switch (pingData->unit) {
    case PING_MM:
      maxPeriod = maxDistance/PING_US_TO_MM;
      break;
    case PING_INCHES:
      maxPeriod = maxDistance/PING_US_TO_INCH;
      break;
    default:
      // Assume distance in micro-seconds
      maxPeriod = (float) maxDistance;
  }
  if (!ping_pingUs(pingData, maxPeriod, &echoTime)) {
    //os_printf("ping_ping failed: maxPeriod=%d echoTime=%d\n",maxPeriod, (int)echoTime );
    return false;
  }
  switch (pingData->unit) {
    case PING_MM:
      *returnDistance = ((float) echoTime)*PING_US_TO_MM;
      break;
    case PING_INCHES:
      *returnDistance = ((float) echoTime)*PING_US_TO_INCH;
      break;
    default:
      // Assume distance in micro-seconds
      *returnDistance = (float) echoTime;
  }
  return true;
}

/**
 * Initiates the GPIO for one-pin mode.
 */
bool ICACHE_FLASH_ATTR
ping_initOnePinMode(Ping_Data *pingData, int8_t triggerAndEchoPin, Ping_Unit unit) {
  return ping_init(pingData, triggerAndEchoPin, triggerAndEchoPin, unit);
}

/**
 * Initiates the Ping_Data structure and sets the GPIOs
 */
bool ICACHE_FLASH_ATTR
ping_init(Ping_Data *pingData, int8_t triggerPin, int8_t echoPin, Ping_Unit unit) {
  pingData->triggerPin = triggerPin;
  pingData->echoPin = echoPin;
  pingData->unit = unit;
  bool singlePinMode = false;

  if (triggerPin == echoPin) {
    singlePinMode = true;
  } else {
    if (!easygpio_pinMode(pingData->triggerPin, EASYGPIO_NOPULL, EASYGPIO_OUTPUT)){
      os_printf("ping_init: Error: failed to set pinMode on trigger pin\n");
      return false;
    }
    GPIO_OUTPUT_SET(pingData->triggerPin, PING_TRIGGER_DEFAULT_STATE);
  }

  if (easygpio_attachInterrupt(pingData->echoPin, EASYGPIO_NOPULL, ping_intr_handler)) {
    ping_allEchoPins |= BIT(pingData->echoPin);
    if (singlePinMode) {
      // easygpio_attachInterrupt() disables output, enable it again
      GPIO_OUTPUT_SET(pingData->triggerPin, PING_TRIGGER_DEFAULT_STATE);
    }
    os_printf("\nInitiated ping module with trigger pin=%d echo pin=%d.\n\n",pingData->triggerPin, pingData->echoPin);
    pingData->isInitiated = true;
  } else {
    os_printf("ping_init: Error: failed to set interrupt on echo pin %d\n", pingData->echoPin);
    pingData->isInitiated = false;
  }
  return pingData->isInitiated;
}
