// Licensed under the MIT License. See LICENSE.txt file in the project root.

#pragma once

#include "irq_guard.h"
#include "cxx_duration.h"

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
    Cnt_ms  = 256,

    Freq    = 1000000 / 1024, // 1MHz / 1024

    Tick_ms = 1,
    Tick_us = 1024,

    Cnt_freq = 1000 / Cnt_ms,
    Max_tick = Freq / Cnt_freq,

    Max_tc = 249,
  };

  __uint24 _cnt = 0;

  using Cnt_type = cxx::qseconds_bits<24>;

  Cnt_type cnt() const
  {
    cxx::Irq_guard g;
    return _cnt;
  }

  Cnt_type cnt_locked() const
  {
    return _cnt;
  }

  cxx::milliseconds now() const
  {
    union {
      uint32_t r;
      struct {
        uint8_t x;
        __uint24 t;
      };
    } n;

    uint8_t c = TCNT0;
      {
        cxx::Irq_guard g;
        n.t = _cnt;
      }
    n.x = TCNT0;
    if (n.x < c)
      n.r += (1 << 8);

    return cxx::milliseconds(n.r);
  }
};

}
