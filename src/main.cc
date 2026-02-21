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

#if USE_I2C
I2c_master I2c_master::m;
Display Display::d;

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

#endif


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
#if USE_I2C
    Display::d.off<i2c_finish_cmds>(i2c_start_cmds);
#endif
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


static cxx::Debounce<PWR_BTN_MSK, cxx::Timer::Hires_type<16>(10), true> pwr_btn;
static cxx::Debounce<ACC_IN_MSK, cxx::Timer::Hires_type<16>(10)> acc_in;

static uint8_t _pin_changed = false;
ISR(PCINT0_vect)
{
  _pin_changed = true;
}

// 30minutes timeout
typedef Timed_pwr_on<cxx::Timer, PWR_DOWN_DELAY_SEC> Tmr;
static Tmr timed_pwr;

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
  init_clk();
  init_timer1();
#if USE_I2C
  I2c_master::m.init();
  Display::d.init();
#endif

  set_sleep_mode(SLEEP_MODE_IDLE | _SLEEP_ENABLE_MASK);

  sei();
  DDRB  |= ACC_OUT_MSK; // ACC_OUT as output
  PORTB |= PWR_BTN_MSK; // pullup power btn pin

  {
    // after configuring the IOs initilize the debouncers
    uint8_t pinb = PINB;
    acc_in.init(pinb);
    pwr_btn.init(pinb);
  }

  GIMSK = 1 << 5;
  PCMSK = ACC_IN_MSK | PWR_BTN_MSK;

  for (;;) {
    uint8_t pinb = PINB;
    auto now = timer.now();
    if (acc_in.update(now, pinb)
        && acc_in.pressed() != 0)
      timed_pwr.acc_update(acc_in, cxx::duration_cast<Tmr::Cnt_type>(now));

    if (pwr_btn.update(now, pinb)
        && (pwr_btn.pressed() < 0)) // release
      {
        timed_pwr.power_btn(acc_in, cxx::duration_cast<Tmr::Cnt_type>(now));
      }

    // if we hit the timeout, handle it
    if (timed_pwr.timeout(cxx::duration_cast<Tmr::Cnt_type>(now)))
      timed_pwr.hit();

#if USE_I2C
    I2c_master::m.step();

    if (timed_pwr.is_ticking() && !I2c_master::m.busy())
      {
        auto eta = timed_pwr.eta(cxx::duration_cast<Tmr::Cnt_type>(now));
        auto min = cxx::duration_cast<cxx::minutes8>(eta);
        auto sec = cxx::duration_cast<cxx::seconds8>(eta - cxx::duration_cast<Tmr::Cnt_type>(min));
        uint16_t sec2 = sec.count() / 10;
        uint16_t sec1 = sec.count() - (sec2 * 10);
        uint16_t min2 = min.count() / 10;
        uint16_t min1 = min.count() - (min2 * 10);
        Display::d.time<i2c_finish_cmds>(sec1 | (sec2 << 4) | (min1 << 8) | (min2 << 12), i2c_start_cmds);
      }
#endif

    if (!pwr_btn.might_sleep()
        || !acc_in.might_sleep()
        || !timed_pwr.might_sleep()
#if USE_I2C
        || !I2c_master::m.might_sleep()
#endif
        )
      continue;

    {
      cxx::Irq_guard g;
      if (wakeup_pending()) {
        clear_wakeups();
        continue;
      }

      // power down if we have no timer running
      if (pwr_btn.might_power_down()
          && acc_in.might_power_down()
          && timed_pwr.might_power_down()
#if USE_I2C
          && I2c_master::m.might_power_down()
#endif
          )
        set_sleep_mode(SLEEP_MODE_PWR_DOWN | _SLEEP_ENABLE_MASK);

      // sleep
      do_sleep();
      clear_wakeups();
      // switch to idle sleep mode
      set_sleep_mode(SLEEP_MODE_IDLE | _SLEEP_ENABLE_MASK);
    }
  }
}
