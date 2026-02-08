// Licensed under the MIT License. See LICENSE.txt file in the project root.

#pragma once

#include "irq_guard.h"

namespace cxx {

/**
 * Timer counter helper for AVR.
 *
 * Manages a counter on top of the AVR timer counter unit.
 * Hnadles the atomic access to the ounter as well as an intrnal representation for the
 * 40bit counter value...
 */
struct Timer
{
  enum
  {
    Cnt_ms  = 250,

    Freq    = 1000000 / 1024, // 1MHz / 1024

    Tick_ms = 1,
    Tick_us = 1024,

    Cnt_freq = 1000 / Cnt_ms,
    Max_tick = Freq / Cnt_freq,
  };

  unsigned long _cnt = 0;

  unsigned long ticks() const
  {
    unsigned long c;
    cxx::Irq_guard g;
    {
      c = _cnt << 8;
    }
    return c + TCNT0;
  }

  unsigned long cnt() const
  {
    cxx::Irq_guard g;
    return _cnt;
  }

  unsigned long cnt_locked() const
  {
    return _cnt;
  }

  struct Time
  {
    uint8_t tc;
    unsigned long cnt;

    unsigned long ticks() const { return (cnt << 8) | tc; }
  };

  Time now() const {
    Time t;
    t.cnt = cnt();
    t.tc = TCNT0;
    return t;
  }
};

}
