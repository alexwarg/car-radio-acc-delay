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

#include <stddef.h>

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/pgmspace.h>

#include "timer.h"
#include "irq_guard.h"
#include "debounce.h"
#include "cxx_duration.h"

#include "cxx_coroutine.h"

enum {
  ACC_OUT_PIN = 4,
  ACC_IN_PIN  = 3,
  PWR_BTN_PIN = 1,

  PIN_SCL = 2,
  PIN_SDA = 0,

  ACC_OUT_MSK = 1 << ACC_OUT_PIN,
  ACC_IN_MSK  = 1 << ACC_IN_PIN,
  PWR_BTN_MSK = 1 << PWR_BTN_PIN,
};
enum : unsigned long
{
  PWR_DOWN_DELAY_SEC = 30ul * 60ul, // poer off timer for manually powerd on mode (without ACC on)
  ACC_DOWN_DELAY_SEC = 20, // delay power down after ACC off for n seconds
};


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
};

static cxx::Timer timer;

static void init_clk()
{
  CLKPR = 0x80;
  CLKPR = 0x03; // prescaler: 8 -> 1MHz
}

static void init_timer()
{
  //TCCR0A = 0x02; // Clear Timer on Compare match
  TCCR0A = 0x00; // normal overrun mode
  TCCR0B = 0x05; // normal, 1024 prescaler
  //OCR0A  = timer.Max_tick - 1;
  TIFR = 0xff;
  //TIMSK = 0x10; // COMP 0A IRQ
  TIMSK = 0x02; // OVFL 0A
  GTCCR = 0;
}

//ISR(TIMER0_COMPA_vect)
ISR(TIMER0_OVF_vect)
{
  asm volatile ("" : "=m"(timer._cnt));
  ++timer._cnt;
  asm volatile ("" : : "m"(timer._cnt));
}


static cxx::Debounce<PWR_BTN_MSK, 10, true> pwr_btn;
static cxx::Debounce<ACC_IN_MSK, 10> acc_in;

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

struct promise;

struct coro : cxx::coroutine_handle<promise>
{
  using promise_type = ::promise;
};

struct promise
{
  coro get_return_object() { return {coro::from_promise(*this)}; }
  cxx::suspend_never initial_suspend() noexcept { return {}; }
  cxx::suspend_never final_suspend() noexcept { return {}; }
  void return_void() {}
  void unhandled_exception() {}

  //static char _s[12];
  //static void *operator new (size_t s) { return &_s; }
  //static void operator delete (void *, size_t s) {};
};

template<typename T>
struct task
{
  struct promise_type
  {
    std::coroutine_handle<> prec;
    T data;

    struct _awaiter
    {
      bool await_ready() const noexcept { return false; }
      void await_resume() const noexcept {}
      std::coroutine_handle<> await_suspend(std::coroutine_handle<promise_type> h)
      {
        if (auto p = h.promise().prec)
          return p;
        return std::coroutine_handle<>{}; //std::noop_coroutine();
      }
    };

    task get_return_object() noexcept
    {
      return {std::coroutine_handle<promise_type>::from_promise(*this)};
    }

    std::suspend_never initial_suspend() const noexcept { return {}; }
    void unhandled_exception() {}
    _awaiter final_suspend() const noexcept { return _awaiter{}; }

    void return_value(T val) noexcept
    {
      data = cxx::move(val);
    }
  };

  bool await_ready() const noexcept
  {
    return handle.done();
  }

  T await_resume() const noexcept
  {
    return cxx::move(handle.promise().data);
  }

  void await_suspend(std::coroutine_handle<> coro) const noexcept
  {
    handle.promise().prec = coro;
  }

  std::coroutine_handle<promise_type> handle;
};

template<>
struct task<void>
{
  struct promise_type
  {
    std::coroutine_handle<> prec;

    struct _awaiter
    {
      bool await_ready() const noexcept { return false; }
      void await_resume() const noexcept {}
      std::coroutine_handle<> await_suspend(std::coroutine_handle<promise_type> h)
      {
        if (auto p = h.promise().prec)
          return p;
        return std::coroutine_handle<>{}; //std::noop_coroutine();
      }
    };

    task<void> get_return_object() noexcept
    {
      return {std::coroutine_handle<promise_type>::from_promise(*this)};
    }

    std::suspend_never initial_suspend() const noexcept { return {}; }
    void unhandled_exception() {}
    _awaiter final_suspend() const noexcept { return _awaiter{}; }

    void return_void() noexcept
    {}
  };

  bool await_ready() const noexcept
  {
    return handle.done();
  }

  void await_resume() const noexcept
  {}

  void await_suspend(std::coroutine_handle<> coro) const noexcept
  {
    handle.promise().prec = coro;
  }

  std::coroutine_handle<promise_type> handle;
};

struct i2c_awaiter
{
  constexpr bool await_ready() const noexcept { return false; }
  void await_suspend(std::coroutine_handle<> h) const noexcept
  { waiting = h; }
  constexpr void await_resume() const noexcept
  {
    waiting = std::coroutine_handle<>{};
  }

  static std::coroutine_handle<> waiting;
};

std::coroutine_handle<> i2c_awaiter::waiting;

[[gnu::always_inline]] inline
task<void> wait_scl_high()
{
  while ( !(PINB & (1 << PIN_SCL)))
    co_await i2c_awaiter();
}

[[gnu::always_inline]]
inline task<void> wait_delay()
{
  co_await i2c_awaiter();
  co_return;
}

inline task<void> i2c_start()
{
  PORTB |= (1 << PIN_SDA);
  PORTB |= (1 << PIN_SCL);
  co_await wait_scl_high();
  PORTB &= ~(1 << PIN_SDA);
  co_await wait_delay();
  PORTB &= ~(1 << PIN_SCL);
  PORTB |= (1 << PIN_SDA);
  co_return;
}

inline task<void> i2c_stop()
{
  PORTB &= ~(1 << PIN_SDA);
  PORTB |= (1 << PIN_SCL);
  co_await wait_scl_high();
  co_await wait_delay();
  PORTB |= (1 << PIN_SDA);
  co_return;
}

inline task<uint8_t> i2c_do_xfer()
{
  do
    {
      co_await wait_delay();
      USICR |= (1 << USITC);
      co_await wait_scl_high();
      co_await wait_delay();
      USICR |= (1 << USITC);
    }
  while (!(USISR & (1 << USIOIF)));
  co_await wait_delay();
  uint8_t data = USIDR;
  USIDR = 0xff;
  co_return data;
}

inline task<uint8_t> i2c_xfer(uint8_t usisr)
{
  PORTB &= ~(1 << PIN_SCL);
  USISR = usisr;
  return i2c_do_xfer();
}

inline task<uint8_t> i2c_do_write_byte()
{
  co_await i2c_xfer(0b11110000);
  DDRB &= ~(1 << PIN_SDA);
  uint8_t nack = co_await i2c_xfer(0b11111110);
  DDRB |= (1 << PIN_SDA);
  co_return nack;
}

inline task<uint8_t> i2c_write_byte(uint8_t byte)
{
  USIDR = byte;
  return i2c_do_write_byte();
}

inline void i2c_init()
{
  DDRB |= (1 << PIN_SDA);
  DDRB |= (1 << PIN_SCL);

  PORTB |= (1<<PIN_SCL);
  PORTB |= (1<<PIN_SDA);

  USIDR = 0xFF;

  USICR = (1 << USIWM1) | (1 << USICS1) | (1 << USICLK);

  USISR = (1 << USISIF) | (1 << USIOIF) | (1 << USIPF) |
    (1 << USIDC) |    // Clear flags,
    (0x0 << USICNT0); // and reset counter.
}

static task<void> i2c_send_seq(uint8_t const *pgm_seq, uint8_t l)
{
  co_await i2c_start();
  co_await i2c_write_byte(0x78);
  co_await i2c_write_byte(0x00);
  for (;l > 0; --l, ++pgm_seq)
    co_await i2c_write_byte(pgm_read_byte(pgm_seq));

  co_return;
}

template<typename T>
class Pgm_ptr
{
  uint8_t const *_p;
  constexpr T operator * () const noexcept { }

};

struct I2C_handler
{
  enum : uint8_t
  {
    S_IDLE        = 0,

    S_START_SCL   = 1,
    S_START_WAIT  = 2,
    S_START_SCL_L = 3,

    S_STOP_SCL   = 4,
    S_STOP_WAIT  = 5,
    S_STOP_WAIT_2 = 6,

    S_SEND_START,
    S_SEND_WAIT_1,
    S_SEND_WAIT_SCL,
    S_SEND_WAIT_2,
    S_SEND_SCL_L,
    S_SEND_WAIT_3,
    S_SEND_R_ACK,
    S_SEND_R_ACK_2,
    S_SEND_R_ACK_SCL,
    S_SEND_R_ACK_WAIT_2,
    S_SEND_R_ACK_SCL_L,
    S_SEND_R_ACK_WAIT_3,
    S_SEND_R_ACK_CHECK
  };

  enum Cmd : uint8_t
  {
    C_end = 0,
    C_start = 1,
    C_stop = 2,
    C_send_bytes = 3,

  };

  uint8_t _state;
  uint8_t const *cmd;

  uint8_t len;
  uint8_t const *buf;

  uint8_t _stop()
  {
    PORTB &= ~(1 << PIN_SDA);
    PORTB |= (1 << PIN_SCL);
    _state = S_STOP_SCL;
    return 1;
  }

  uint8_t _send_start()
  {
    _state = S_SEND_START;
    return 1;
  }

  uint8_t next_cmd()
  {
    switch (pgm_read_byte(cmd++)) {
      case C_end:
        _state = S_IDLE;
        return 0;
      case C_start:
        // generate start condition
        PORTB |= (1 << PIN_SDA);
        PORTB |= (1 << PIN_SCL);
        _state = S_START_SCL;
        return 1;

      case C_stop:
        return _stop();

      case C_send_bytes:
        len = pgm_read_byte(cmd++);
        buf = (uint8_t const *)pgm_read_word(cmd);
        cmd += 2;
        return _send_start();
    }
  }

  uint8_t step()
  {
    switch (_state) {
      case S_IDLE:
        return 0;

      case S_START_SCL:
        if (PINB & (1 << PIN_SCL))
          {
            PORTB &= ~(1 << PIN_SDA);
            _state = S_START_WAIT;
          }
        return 1;

      case S_START_WAIT:
        _state = S_START_SCL_L;
        return 1;

      case S_START_SCL_L:
        PORTB &= ~(1 << PIN_SCL);
        PORTB |= (1 << PIN_SDA);
        return next_cmd();

      case S_STOP_SCL:
        if (!(PINB & (1 << PIN_SCL)))
          return 1;
        _state = S_STOP_WAIT;
        return 1;

      case S_STOP_WAIT:
        PORTB |= (1 << PIN_SDA);
        _state = S_STOP_WAIT_2;
        return 1;

      case S_STOP_WAIT_2:
        return next_cmd();

      case S_SEND_START:
        USISR = 0b11110000;
        USIDR = pgm_read_byte(buf++);
        len--;
        _state = S_SEND_WAIT_1;
        return 1;

      case S_SEND_WAIT_1:
        USICR |= (1 << USITC);
        _state = S_SEND_WAIT_SCL;
        return 1;

      case S_SEND_WAIT_SCL:
        if (!(PINB & (1 << PIN_SCL)))
          _state = S_SEND_WAIT_2;
        return 1;

      case S_SEND_WAIT_2:
        _state = S_SEND_SCL_L;
        return 1;

      case S_SEND_SCL_L:
        USICR |= (1 << USITC);
        if (!(USISR & (1 << USIOIF)))
          _state = S_SEND_WAIT_1;
        else
          _state = S_SEND_WAIT_3;
        return 1;

      case S_SEND_WAIT_3:
        USIDR = 0xff;
        USISR = 0b11111110;
        DDRB &= ~(1 << PIN_SDA);
        _state = S_SEND_R_ACK;
        return 1;

      case S_SEND_R_ACK:
        _state = S_SEND_R_ACK_2;
        return 1;

      case S_SEND_R_ACK_2:
        USICR |= (1 << USITC);
        _state = S_SEND_R_ACK_SCL;
        return 1;

      case S_SEND_R_ACK_SCL:
        if (!(PINB & (1 << PIN_SCL)))
          _state = S_SEND_R_ACK_WAIT_2;
        return 1;

      case S_SEND_R_ACK_WAIT_2:
        _state = S_SEND_R_ACK_SCL_L;
        return 1;

      case S_SEND_R_ACK_SCL_L:
        USICR |= (1 << USITC);
        _state = S_SEND_R_ACK_WAIT_3;
        return 1;

      case S_SEND_R_ACK_WAIT_3:
        _state = S_SEND_R_ACK_CHECK;
        return 1;

      case S_SEND_R_ACK_CHECK:
          {
            uint8_t t = USIDR;
            DDRB |= (1 << PIN_SDA);
            if (t)
              return _stop();
          }

        if (len > 0)
          return _send_start();

        return next_cmd();
    }
  }


  bool might_sleep() const { return !(_state & 0x80); }
  bool might_power_down() const { return _state == S_IDLE; }
};


int main()
{
  init_clk();
  init_timer();
  i2c_init();

  set_sleep_mode(SLEEP_MODE_IDLE | _SLEEP_ENABLE_MASK);

  sei();
  DDRB  |= ACC_OUT_MSK; // ACC_OUT as output
  PORTB |= PWR_BTN_MSK; // pullup power btn pin

  auto i2c = i2c_send_seq(nullptr, 0);
  //coro i2c = ([]() -> coro { co_return; })();

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
      timed_pwr.power_btn(acc_in, cxx::duration_cast<Tmr::Cnt_type>(now));

    // if we hit the timeout, handle it
    if (timed_pwr.timeout(cxx::duration_cast<Tmr::Cnt_type>(now)))
      timed_pwr.hit();

    if (i2c_awaiter::waiting)
      i2c_awaiter::waiting.resume();

    if (!pwr_btn.might_sleep()
        || !acc_in.might_sleep()
        || !timed_pwr.might_sleep())
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
          && timed_pwr.might_power_down())
        set_sleep_mode(SLEEP_MODE_PWR_DOWN | _SLEEP_ENABLE_MASK);

      // sleep
      do_sleep();
      clear_wakeups();
      // switch to idle sleep mode
      set_sleep_mode(SLEEP_MODE_IDLE | _SLEEP_ENABLE_MASK);
    }
  }
}
