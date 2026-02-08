// Licensed under the MIT License. See LICENSE.txt file in the project root.

#pragma once

#include <avr/io.h>

namespace cxx {

/**
 * Input pin debounce helper.
 *
 * Handles a potentially unstable / noisy input pin and does a debouncing in software.
 * This includes the state tracking and a timeout handling.
 */
template<uint8_t MSK, unsigned long DELAY = 5, bool NEG = false>
struct Debounce
{
  unsigned long db_time;
  bool _last:1;
  bool _old:1;
  bool _running:1;

  Debounce(uint8_t pv = PINB)
  {
    _last = !!(pv & MSK);
    _old = _last;
    _running = false;
  }

  void init(uint8_t pv = PINB)
  {
    _last = !!(pv & MSK);
    _old = _last;
    _running = false;
  }


  bool running() const { return _running; }
  bool might_sleep() const { return !_running; }
  // if we might sleep we can power down (PCINT wakes us up)
  bool might_power_down() const { return true; }

  bool update(unsigned long ts, uint8_t pv = PINB)
  {
    if ((!!(pv & MSK)) != _last) {
      db_time = ts;
      _running = true;
      _last = !!(pv & MSK);
      return false;
    } else if (!_running) {
      return true;
    } else if ((ts - db_time) > DELAY) {
      _running = false;
      return true;
    }
    return false;
  }

  bool state() const
  { return NEG ^ _last; }

  int8_t pressed()
  {
    if (_old == _last)
      return 0;

    bool o = _old;
    _old = _last;
    return (NEG ^ o) ? -1 : 1;
  }
};

}
