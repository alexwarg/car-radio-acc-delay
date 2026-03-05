// Licensed under the MIT License. See LICENSE.txt file in the project root.

#pragma once

#include <avr/io.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>

struct Sleep_support
{
  static void prepare_power_down()
  {
    set_sleep_mode(SLEEP_MODE_PWR_DOWN | _SLEEP_ENABLE_MASK);
  }

  static void prepare_idle_sleep()
  {
    set_sleep_mode(SLEEP_MODE_IDLE | _SLEEP_ENABLE_MASK);
  }

  static void do_sleep()
  {
    asm volatile ("" : : : "memory");
    sei();
    sleep_cpu();
    cli();
  }
};


