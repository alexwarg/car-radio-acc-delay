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
template<uint8_t MSK,
  auto DELAY = cxx::milliseconds16(10),
  bool NEG = false>
struct Debounce
{
  using delay_type = decltype(DELAY);

protected:
  delay_type db_time;

  static constexpr delay_type Delay = DELAY;

  uint8_t _state;

  enum : uint8_t
  {
    S_cur    = 0x01,
    S_old    = 0x02,
    S_db_run = 0x04,
    S_usr_1  = 0x08,
    S_usr_2  = 0x10,
    S_usr_3  = 0x20,
    S_usr_4  = 0x40,
    S_usr_5  = 0x80
  };

  void state_modify(uint8_t del, uint8_t add)
  { _state = (_state & ~del) | add; }

  void state_add(uint8_t s)
  { _state |= s; }

  void state_del(uint8_t s)
  { _state &= ~s; }

  bool have_state(uint8_t s) const
  { return _state & s; }

public:
  Debounce() = default;

  void init(uint8_t pv = PINB)
  {
    bool s = pv & MSK;
    _state = s ? (S_cur | S_old) : 0;
  }

  explicit Debounce(uint8_t pv)
  {
    init(pv);
  }

  bool might_sleep() const { return !have_state(S_db_run); }
  // if we might sleep we can power down (PCINT wakes us up)
  bool might_power_down() const { return true; }

  bool update(delay_type const &ts, uint8_t pv = PINB)
  {
    uint8_t val = (pv & MSK) ? S_cur : 0;
    if (val != (_state & S_cur)) {
      db_time = ts + Delay;
      state_modify(S_cur, S_db_run | val);
      return false;
    } else if (!have_state(S_db_run)) {
      return true;
    } else if (ts > db_time) {
      state_del(S_db_run);
      return true;
    }
    return false;
  }

  bool state() const
  { return NEG ^ have_state(S_cur); }

  int8_t pressed()
  {
    bool cur = have_state(S_cur);
    bool old = have_state(S_old);
    if (old == cur)
      return 0;

    state_modify(S_old, cur ? S_old : 0);
    return (NEG ^ old) ? -1 : 1;
  }

  int8_t pressed(delay_type const &)
  { return pressed(); }
};

}
