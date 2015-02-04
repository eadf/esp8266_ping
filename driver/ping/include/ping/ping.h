/*
* ping.h
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

#ifndef PING_INCLUDE_PING_PING_H_
#define PING_INCLUDE_PING_PING_H_

#include "c_types.h"

#define PING_US_TO_MM (1.0/5.8)
#define PING_US_TO_INCH (1.0/148.0)

typedef enum {
  PING_MM = 0,
  PING_INCHES
} Ping_Unit;

/**
 * Sends a ping, and returns the number of microseconds it took to receive a response.
 * Will give up after maxPeriod (with false as return value)
 */
bool ping_ping(uint32_t maxPeriod, uint32_t* response);

/**
 * Sends a ping, and returns the response in the specified unit (mm/inches)
 * returns false if no result could be found.
 */
bool ping_pingDistance(Ping_Unit unit, float maxDistance, float* returnDistance);

/**
 * Initiates the GPIOs.
 * Set triggerPin and echoPin to the same value for one-pin mode.
 */
bool ping_init(uint8_t triggerPin, uint8_t echoPin);

/**
 * Initiates the GPIO for one-pin mode.
 */
bool ping_initOnePinMode(uint8_t triggerAndEchoPin);
#endif /* PING_INCLUDE_PING_PING_H_ */
