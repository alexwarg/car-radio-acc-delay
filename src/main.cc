// Licensed under the MIT License. See LICENSE.txt file in the project root.

/**
 * ACC timer
 *
 * The project is designed to take two inputs, an ingition (ACC) signal, and a power button.
 * The output is an ignition (ACC) signal with extra function:
 *  - In case the ACC input transitions from low-to-high the ACC output is switched to high.
 *    As long ass ACC input remains high, the button allows switch off and on of the ACC output.
 *  - In case of ACC input is low, the button allows to toggle the ACC output on and off, with
 *    an additional timer of 30 minutes to automatically switch off the ACC output.
 */

#include <cstddef>

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/pgmspace.h>

#include "timer.h"
#include "irq_guard.h"
#include "debounce.h"
#include "cxx_duration.h"
#include "task_list.h"

#include "i2c.h"
#include "display.h"


enum {
  ACC_OUT_PIN = 4,
  ACC_IN_PIN  = 3,
  PWR_BTN_PIN = 1,

  ACC_OUT_MSK = 1 << ACC_OUT_PIN,
  ACC_IN_MSK  = 1 << ACC_IN_PIN,
  PWR_BTN_MSK = 1 << PWR_BTN_PIN,
};

enum : unsigned long
{
  PWR_DOWN_DELAY_SEC = 30 * 60ul, // poer off timer for manually powerd on mode (without ACC on)
  ACC_DOWN_DELAY_SEC = 0, // delay power down after ACC off for n seconds
};

#define USE_I2C 0

template<typename TIMER, unsigned long ON_TIME_SECS>
struct Timed_pwr_on
{
  using Cnt_max_type = cxx::qseconds<~0ul>;
  static constexpr cxx::seconds On_time = ON_TIME_SECS;
  static constexpr cxx::milliseconds On_time_ms = On_time;
  static constexpr Cnt_max_type On_time_qs = cxx::duration_cast<Cnt_max_type>(On_time);
  using Cnt_type = cxx::qseconds<2 * On_time_qs.count()>;
  static constexpr Cnt_type On_time_diff = On_time_qs;

  static constexpr cxx::seconds Acc_delay = ACC_DOWN_DELAY_SEC;
  static constexpr Cnt_max_type Acc_delay_qs = cxx::duration_cast<Cnt_max_type>(Acc_delay);
  static constexpr Cnt_type Acc_delay_diff = Acc_delay_qs;

  Cnt_type _pwr_off_time;
  uint8_t _pwr;

  enum
  {
    P_off   = 0,
    P_acc   = 1,
    P_timer = 2,
  };

  bool is_on() const
  {
    return _pwr != P_off;
  }

  bool is_ticking() const
  {
    return _pwr == P_timer;
  }

  Cnt_type eta(Cnt_type now) const
  { return _pwr_off_time - now; }


  bool timeout(Cnt_type now) const
  {
    if (_pwr != P_timer)
      return false;

    using Scnt = cxx::signed_type_t<Cnt_type>;

    if (static_cast<Scnt>(now - _pwr_off_time) >= Scnt::zero())
      return true;

    return false;
  }

  void hit()
  {
    _pwr = P_off;
    switch_acc_off();
  }

  void start_timer(Cnt_type timer)
  {
    _pwr_off_time = timer;
    _pwr = P_timer;
  }

  bool stop_timer()
  {
    if (_pwr != P_timer)
      return false;

    _pwr = P_off;
    return true;
  }

  bool running() const { return _pwr == P_timer; }
  // timers always can sleep, just might not power down...
  bool might_sleep() const { return true; }
  // timers always can sleep, just might not power down...
  bool might_power_down() const { return !running(); }

  void switch_acc_on()
  {
    PORTB |= ACC_OUT_MSK;
  }

  void switch_acc_off()
  {
    PORTB &= ~ACC_OUT_MSK;
  }

  template<typename ACC>
  bool power_btn(ACC const &acc, Cnt_type now)
  {
    switch (_pwr)
    {
      case P_off:
        if (acc.state())
          _pwr = P_acc;
        else
          start_timer(now + On_time_diff);

        switch_acc_on();
        return false;
      case P_timer:
        stop_timer();
        switch_acc_off();
        return true;
      case P_acc:
        _pwr = P_off;
        switch_acc_off();
        return true;
    }
    return false;
  }

  template<typename ACC>
  void acc_update(ACC const &acc, Cnt_type now)
  {
    if (acc.state())
    {
      switch (_pwr)
      {
        default:
          return;
        case P_timer:
          _pwr = P_acc;
          return;
        case P_off:
          _pwr = P_acc;
          switch_acc_on();
          return;
      }
    }
    else
    {
      switch (_pwr)
      {
        case P_off:
        case P_timer:
          return;
        case P_acc:
          if (Acc_delay_diff)
            {
              start_timer(now + Acc_delay_diff);
            }
          else
            {
              _pwr = P_off;
              switch_acc_off();
            }
          return;
      }
    }
  }

  void update(auto const &now, auto &&...)
  {
    // if we hit the timeout, handle it
    if (timeout(cxx::duration_cast<Cnt_type>(now)))
      hit();
  }

  static void init(auto &&...) {}
};

static cxx::Timer timer;

static void init_clk()
{
  CLKPR = 0x80;
  CLKPR = 0x00; // prescaler: 0 -> 8MHz
}

static void init_timer1()
{
  TCCR1 = 0x0e; // normal mode + 8192 prescaler
  TIFR  = 0xff;
  TIMSK = 0x04; // OVFL 1
  GTCCR = 0;
}

//ISR(TIMER0_COMPA_vect)
//ISR(TIMER0_OVF_vect)
ISR(TIMER1_OVF_vect)
{
  asm volatile ("" : "=m"(timer._cnt));
  ++timer._cnt;
  asm volatile ("" : : "m"(timer._cnt));
}

static uint8_t _pin_changed = false;
ISR(PCINT0_vect)
{
  _pin_changed = true;
}

// 30minutes timeout
typedef Timed_pwr_on<cxx::Timer, PWR_DOWN_DELAY_SEC> Tmr;
static Tmr timed_pwr;

template<typename IN = cxx::Debounce<ACC_IN_MSK, cxx::Timer::Hires_type<16>(10)>>
struct Acc_input : IN
{
  void update(auto const &now, uint8_t pv)
  {
    if (IN::update(now, pv) && IN::pressed() != 0)
      timed_pwr.acc_update(*this, cxx::duration_cast<Tmr::Cnt_type>(now));
  }
};

static Acc_input acc_in;

template<typename IN = cxx::Debounce<PWR_BTN_MSK, cxx::Timer::Hires_type<16>(10), true>>
struct Pwr_btn : IN
{
  void update(auto const &now, uint8_t pv)
  {
    if (!IN::update(now, pv))
      return;

    switch (IN::pressed(now))
      {
      case 0:
      default:
        break;
      case 1:
        timed_pwr.power_btn(acc_in, cxx::duration_cast<Tmr::Cnt_type>(now));
        break;
      }
  }
};

static Pwr_btn pwr_btn;

#if USE_I2C

I2c_master I2c_master::m;
Display Display::d;


struct Display_task
{
  static I2c_master::Start_ptr i2c_queue;
  static void i2c_start_cmds(I2c_master::Start_ptr p)
  {
    if (I2c_master::m.busy())
      i2c_queue = p;
    else
      I2c_master::m.start_cmds(p);
  }

  static void i2c_finish_cmds(uint8_t)
  {
    if (!i2c_queue)
      return;

    auto tmp = i2c_queue;
    i2c_queue = nullptr;
    I2c_master::m.start_cmds(tmp);
  }

  static void init(auto &&...)
  {
    I2c_master::m.init();
    Display::d.init();
  }

  static void update(auto const &now, auto)
  {
    I2c_master::m.step();

    if (timed_pwr.is_ticking() && !I2c_master::m.busy())
      {
        Display::d._on = 1;
        // assume eta is (far) less than 4h == 240 * 60 seconds (16bit is big enough)
        auto eta = cxx::duration_cast<cxx::seconds16>(timed_pwr.eta(cxx::duration_cast<Tmr::Cnt_type>(now)));
        auto min = cxx::duration_cast<cxx::minutes8>(eta);
        auto sec = cxx::duration_cast<cxx::seconds8>(eta - min);
        uint16_t sec2 = sec.count() / 10;
        uint16_t sec1 = sec.count() - (sec2 * 10);
        uint16_t min2 = min.count() / 10;
        uint16_t min1 = min.count() - (min2 * 10);
        Display::d.time<i2c_finish_cmds>(sec1 | (sec2 << 4) | (min1 << 8) | (min2 << 12), i2c_start_cmds);
      }
    else if(!timed_pwr.is_ticking() && Display::d._on)
      {
        Display::d._on = 0;
        Display::d.off<i2c_finish_cmds>(i2c_start_cmds);
      }
  }

  static bool
  might_sleep() { return I2c_master::m.might_sleep(); }

  static bool
  might_power_down() { return I2c_master::m.might_power_down(); }
};

I2c_master::Start_ptr Display_task::i2c_queue;

using Tasks = Task_list<Acc_input<> &, Pwr_btn<> &, Tmr &, Display_task>;

#else // USE_I2C == 0 ... no display support

using Tasks = Task_list<Acc_input<> &, Pwr_btn<> &, Tmr &>;

#endif // USE_I2C == 0


static void do_sleep()
{
  asm volatile ("" : : : "memory");
  sei();
  sleep_cpu();
  cli();
}

static bool wakeup_pending()
{
  return timed_pwr.timeout(timer.cnt_locked())
         || _pin_changed;
}

static void clear_wakeups()
{
  _pin_changed = false;
}


int main()
{
  Tasks tasks { acc_in, pwr_btn, timed_pwr };

  init_clk();
  init_timer1();

  set_sleep_mode(SLEEP_MODE_IDLE | _SLEEP_ENABLE_MASK);

  DDRB  |= ACC_OUT_MSK; // ACC_OUT as output
  PORTB |= PWR_BTN_MSK; // pullup power btn pin

  // after configuring the IOs initilize the debouncers
  tasks.init(uint8_t(PINB));

  sei();

  GIMSK = 1 << 5;
  PCMSK = ACC_IN_MSK | PWR_BTN_MSK;

  for (;;)
    {
      tasks.update(timer.now(), uint8_t(PINB));

      if (!tasks.might_sleep())
        continue;

      // irq guard scope
        {
          cxx::Irq_guard g;
          if (wakeup_pending())
            {
              clear_wakeups();
              continue;
            }

          if (tasks.might_power_down())
            set_sleep_mode(SLEEP_MODE_PWR_DOWN | _SLEEP_ENABLE_MASK);

          // sleep
          do_sleep();
          clear_wakeups();
          // switch to idle sleep mode
          set_sleep_mode(SLEEP_MODE_IDLE | _SLEEP_ENABLE_MASK);
        }
    }
}
