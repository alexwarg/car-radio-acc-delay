
// Licensed under the MIT License. See LICENSE.txt file in the project root.

#include "debounce.h"
#include "cxx_duration.h"

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
struct Btn : private Debounce<MSK, DELAY, NEG>
{
  using Base = Debounce<MSK, DELAY, NEG>;
  using delay_type = decltype(DELAY); //typename Base::delay_type;
  using Base::Base;
  using Base::init;
  using Base::update;
  using Base::might_power_down;

  delay_type press_ts;

  static constexpr delay_type double_click_time = cxx::duration_cast<delay_type>(cxx::milliseconds16(250));
  static constexpr delay_type long_press_time = cxx::duration_cast<delay_type>(cxx::milliseconds16(1000));

  static constexpr uint8_t S_t_run = Base::S_usr_1;
  static constexpr uint8_t S_double_click = Base::S_usr_2;

  bool might_sleep() const
  { return Base::might_sleep() && !this->have_state(S_t_run); }

  int8_t pressed(delay_type const &now)
  {
    int8_t p = Base::pressed();
    if (!this->have_state(S_t_run))
      {
        if (p > 0)
          {
            this->state_add(S_t_run);
            press_ts = now;
          }
        return 0;
      }

    bool in_double_click = (press_ts + double_click_time) > now;
    if ((p > 0) && in_double_click)
      {
        this->state_add(S_double_click); // double click
        return 0; // we might still get a long double click
      }

    if ((p < 0) && in_double_click)
      {
        if (this->have_state(S_double_click))
          {
            this->state_del(S_t_run | S_double_click);
            return 3; // return the double click
          }

        return 0;
      }

    if ((p == 0) && !this->state() && !in_double_click)
      {
        this->state_del(S_t_run);
        return 1; // return the single click
      }

    if (p > 0)
      return 0;

    bool long_press = now >= (press_ts + long_press_time);
    if ((p < 0) || long_press)
      {
        if (this->have_state(S_double_click))
          {
            this->state_del(S_t_run | S_double_click);
            return 3 + long_press; // long double click;
          }

        this->state_del(S_t_run);
        return 1 + long_press;
      }

    return 0;
  }
};


}
