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
#define PING_TRIGGER_DEFAULT_STATE 1

static volatile uint32_t   ping_timeStamp0 = 0;
static volatile bool       ping_echoStarted = false;
static volatile uint32_t   ping_timeStamp1 = 0;
static volatile bool       ping_echoEnded = false;

static uint8_t ping_triggerPin = 255;
static uint8_t ping_echoPin = 255;

// forward declarations
static void ping_disableInterrupt(void);
static void ping_enableInterrupt(void);
static void ping_intr_handler(void);
static bool ping_isInitiated = false;

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

bool ICACHE_FLASH_ATTR
ping_ping(uint32_t maxPeriod, uint32_t* response) {
  ping_echoEnded = false;
  ping_echoStarted = false;
  ping_timeStamp0 = ping_micros;

  gpio_pin_intr_state_set(GPIO_ID_PIN(ping_echoPin), GPIO_PIN_INTR_POSEDGE);
  GPIO_OUTPUT_SET(ping_triggerPin, !PING_TRIGGER_DEFAULT_STATE);
  os_delay_us(50);
  GPIO_OUTPUT_SET(ping_triggerPin, PING_TRIGGER_DEFAULT_STATE);

  while (!ping_echoEnded) {
    if (ping_micros - ping_timeStamp0 > maxPeriod) {
      *response = ping_micros - ping_timeStamp0;
      return false;
    }
    os_delay_us(10);
  }

  *response = ping_timeStamp1 - ping_timeStamp0;
  if (*response < 100) {
    // false result
    return false;
  }
  return true;
}

void ICACHE_FLASH_ATTR
ping_init(uint8_t pingPin, uint8_t responsePin) {
  ping_triggerPin = pingPin;
  ping_echoPin = responsePin;

  if (easygpio_countBits(1<<ping_triggerPin|1<<ping_echoPin) != 2) {
    os_printf("ping_init you must specify two unique pins to use\n");
    ping_isInitiated = false;
    return;
  }

  easygpio_pinMode(ping_triggerPin, NOPULL, OUTPUT);
  GPIO_OUTPUT_SET(ping_triggerPin, PING_TRIGGER_DEFAULT_STATE);

  if (easygpio_attachInterrupt(ping_echoPin, NOPULL, ping_intr_handler)) {
    ping_isInitiated = true;
  } else {
    os_printf("ping_init failed to set interrupt\n");
    ping_isInitiated = false;
  }

}
