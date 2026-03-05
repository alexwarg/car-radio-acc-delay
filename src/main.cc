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
#include <avr/pgmspace.h>

#include "timer.h"
#include "irq_guard.h"
#include "debounce.h"
#include "cxx_duration.h"
#include "task_list.h"
#include "sleep_support.h"

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

//---------------------------------------------------------
//  main application logic: handling the ACC output pin
//  accoring to input state and timer based power-off...
template<unsigned long ON_TIME_SECS>
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

  enum
  {
    P_off   = 0, //< ACC output is off
    P_acc   = 1, //< ACC output is on due to ACC input on
    P_timer = 2, //< ACC output is on and power-off timer is ticking
  };

  Cnt_type _pwr_off_time;
  uint8_t _pwr;

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

  // task loop API
  using update_time_type = Cnt_type;
  using wakeup_time_type = Cnt_type;

  static void init(auto &&...) {}

  void update(auto const &now, auto &&...)
  {
    // if we hit the timeout, handle it
    if (timeout(cxx::duration_cast<Cnt_type>(now)))
      hit();
  }

  bool wakeup_pending(auto const &now) const
  {
    return timeout(now);
  }

  static void clear_wakeups() {}
};


//---------------------------------------------------------
// Timer / Clock instance
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

//---------------------------------------------------------
// recording pin changed IRQ and translating it into a
// wakeup reason for the task loop
class Pin_changed_task
{
  uint8_t _s;

public:
  using update_time_type = void;
  using wakeup_time_type = void;

  void set() { _s = true; }
  void clear() { _s = false; }
  bool is_set() const { return _s; }

  bool wakeup_pending(auto ...) const
  {
    return is_set();
  }

  void clear_wakeups()
  { clear(); }

  void update(auto &&...) const {}
  void init(auto &&...) const {}

  bool might_sleep() const { return true; }
  bool might_power_down() const { return true; }
};

static Pin_changed_task _pin_changed;
ISR(PCINT0_vect)
{
  _pin_changed.set();
}
//---------------------------------------------------------


//---------------------------------------------------------
// mixin for wrapping a debounced IO pin to be handled in a
// task loop.
// The DERVIVED class needs to provide on_update() function
// for handling updates.
template<typename DERIVED, auto &pin>
struct Io_pin_task
{
  using wakeup_time_type = void;

  void update(auto &&now, uint8_t pv)
  {
    if (!pin.update(now, pv))
      return;

    static_cast<DERIVED *>(this)->on_update(now, pin.pressed(now));
  }

  template<typename ...Args>
  void init(Args &&...args) { pin.init(std::forward<Args>(args)...); }

  bool might_sleep() const { return pin.might_sleep(); }
  bool might_power_down() const { return pin.might_power_down(); }

  void clear_wakeups() const {}
  bool wakeup_pending(auto ...) const { return false; }
};
//---------------------------------------------------------


// 30minutes timeout
using Tmr = Timed_pwr_on<PWR_DOWN_DELAY_SEC>;

static Tmr timed_pwr;
static cxx::Debounce<ACC_IN_MSK, cxx::Timer::Hires_type<16>(10)> acc_in;
static cxx::Debounce<PWR_BTN_MSK, cxx::Timer::Hires_type<16>(10), true> pwr_btn;

//---------------------------------------------------------
// ACC input pin handling
class Acc_in : public Io_pin_task<Acc_in, acc_in>
{
public:
  using update_time_type = cxx::Timer::Time_type; //< needed for task loop
  void on_update(update_time_type const &now, int8_t key)
  {
    if (key != 0)
      timed_pwr.acc_update(acc_in, cxx::duration_cast<Tmr::Cnt_type>(now));
  }
};

//---------------------------------------------------------
// power button handling
class Pwr_btn : public Io_pin_task<Pwr_btn, pwr_btn>
{
public:
  using update_time_type = cxx::Timer::Time_type; //< needed for task loop
  void on_update(update_time_type const &now, int8_t key)
  {
    switch (key)
      {
      case 0:
      default:
        break;
      case 1: // short press / click
        timed_pwr.power_btn(acc_in, cxx::duration_cast<Tmr::Cnt_type>(now));
        break;
      }
  }
};


//---------------------------------------------------------
#if USE_I2C

I2c_master I2c_master::m;
Display Display::d;


//---------------------------------------------------------
// task around i2c and display
struct Display_task
{
  using update_time_type = Tmr::Cnt_type;
  using wakeup_time_type = void;

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

  static void update(auto const &now, auto &&)
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

  static constexpr bool
  wakeup_pending(auto ...) { return false; }

  static void clear_wakeups() {}
};

I2c_master::Start_ptr Display_task::i2c_queue;

using Tasks = Task_list<Acc_in, Pwr_btn, Tmr &, Pin_changed_task &, Display_task>;

#else // USE_I2C == 0 ... no display support

using Tasks = Task_list<Acc_in, Pwr_btn, Tmr &, Pin_changed_task &>;

#endif // USE_I2C == 0

//---------------------------------------------------------
int main()
{
  Tasks tasks { Acc_in{}, Pwr_btn{}, timed_pwr, _pin_changed };

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

  tasks.task_loop(timer, Sleep_support{}, &PINB);
}
