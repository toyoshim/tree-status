/* Name: main.c
 * Project: CrTreeStatus
 * Author: Takashi Toyoshima
 * Creation Date: 2012-01-03
 * Copyright: (c) 2012 by Takashi Toyoshima
 * License: GNU GPL v2 (see License.txt)
 */

#define LED_PORT_DDR        DDRB
#define LED_PORT_OUTPUT     PORTB
#define LED_BIT_GREEN       0
#define LED_BIT_RED         1

#define REQ_PING            0
#define REQ_SET             1
#define REQ_GET             2

#define LED_ON              0
#define LED_OFF             1
#define LED_FLASH           2
#define LED_UNKNOWN         3

#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include <avr/pgmspace.h>
#include "usbdrv.h"

/* ------------------------------------------------------------------------- */
/* ----------------------------- USB interface ----------------------------- */
/* ------------------------------------------------------------------------- */

static uchar animation = _BV(LED_BIT_GREEN) | _BV(LED_BIT_RED);
static uchar led_status_green = LED_FLASH;
static uchar led_status_red = LED_FLASH;
static uchar response_buffer;

usbMsgLen_t usbFunctionSetup(uchar data[8])
{
  usbRequest_t* rq = (void *)data;
  switch (rq->bRequest) {
  case REQ_PING:
    animation = 0;
    LED_PORT_OUTPUT |= _BV(LED_BIT_GREEN) | _BV(LED_BIT_RED);
    led_status_green = LED_OFF;
    led_status_red = LED_OFF;
    response_buffer = rq->wValue.bytes[0];
    usbMsgPtr = &response_buffer;
    return 1;
  case REQ_SET:
    if (rq->wIndex.bytes[0] == LED_BIT_GREEN)
      led_status_green = rq->wValue.bytes[0];
    else if (rq->wIndex.bytes[0] == LED_BIT_RED)
      led_status_red = rq->wValue.bytes[0];
    else
      return 0;
    LED_PORT_OUTPUT |= _BV(LED_BIT_GREEN) | _BV(LED_BIT_RED);
    if (led_status_green == LED_ON)
      LED_PORT_OUTPUT &= ~_BV(LED_BIT_GREEN);
    if (led_status_red == LED_ON)
      LED_PORT_OUTPUT &= ~_BV(LED_BIT_RED);
    animation = 0;
    if (led_status_green == LED_FLASH)
      animation |= _BV(LED_BIT_GREEN);
    if (led_status_red == LED_FLASH)
      animation |= _BV(LED_BIT_RED);
    return 0;
  case REQ_GET:
    if (rq->wIndex.bytes[0] == LED_BIT_GREEN)
      response_buffer = led_status_green;
    else if (rq->wIndex.bytes[0] == LED_BIT_RED)
      response_buffer = led_status_red;
    else
      return 0;
    usbMsgPtr = &response_buffer;
    return 1;
  }
  return 0;
}

/* ------------------------------------------------------------------------- */

int __attribute__((noreturn)) main(void)
{
  unsigned int i;
  wdt_enable(WDTO_1S);
  usbInit();
  usbDeviceDisconnect();
  LED_PORT_DDR |= _BV(LED_BIT_GREEN) | _BV(LED_BIT_RED);
  LED_PORT_OUTPUT &= ~(_BV(LED_BIT_GREEN) | _BV(LED_BIT_RED));
  for (i = 0; i < 250; i++) {
    wdt_reset();
    _delay_ms(1);
  }
  usbDeviceConnect();
  sei();
  for (;;) {
    for(i = 0; i < 65535; i++) {
      wdt_reset();
      usbPoll();
    }
    LED_PORT_OUTPUT ^= animation;
  }
}

/* ------------------------------------------------------------------------- */
