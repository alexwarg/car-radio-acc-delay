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
template<uint8_t MSK, unsigned long DELAY = 10, bool NEG = false>
struct Debounce
{
  using delay_type = cxx::duration<int16_t, cxx::milli>;
  delay_type db_time;

  static constexpr delay_type Delay = DELAY;

  uint8_t _state;

  enum
  {
    S_last = 1,
    S_old  = 2,
    S_running = 4,
  };

  Debounce() = default;

  void init(uint8_t pv = PINB)
  {
    bool s = pv & MSK;
    _state = s ? (S_last | S_old) : 0;
  }

  explicit Debounce(uint8_t pv)
  {
    init(pv);
  }


  bool running() const { return _state & S_running; }
  bool might_sleep() const { return !(_state & S_running); }
  // if we might sleep we can power down (PCINT wakes us up)
  bool might_power_down() const { return true; }

  bool update(delay_type const &ts, uint8_t pv = PINB)
  {
    bool val = pv & MSK;
    if (val != bool(_state & S_last)) {
      db_time = ts + Delay;
      _state = (_state & S_old) | S_running | (val ? S_last : 0);
      return false;
    } else if (!(_state & S_running)) {
      return true;
    } else if (ts > db_time) {
      _state &= ~S_running;
      return true;
    }
    return false;
  }

  bool state() const
  { return NEG ^ bool(_state & S_last); }

  int8_t pressed()
  {
    bool last = _state & S_last;
    bool old  = _state & S_old;
    if (old == last)
      return 0;

    _state = (_state & ~S_old) | (last ? S_old : 0);
    return (NEG ^ old) ? -1 : 1;
  }
};

}
