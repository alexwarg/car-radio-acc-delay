/**
 * Blink
 *
 * Turns on an LED on for one second,
 * then off for one second, repeatedly.
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

enum {
  ACC_OUT_PIN = 4,
  ACC_IN_PIN  = 3,
  PWR_BTN_PIN = 1,

  ACC_OUT_MSK = 1 << ACC_OUT_PIN,
  ACC_IN_MSK  = 1 << ACC_IN_PIN,
  PWR_BTN_MSK = 1 << PWR_BTN_PIN,
};

class Irq_guard
{
  uint8_t f;
public:
  Irq_guard() : f(SREG) { cli(); };
  ~Irq_guard() { SREG = f; }
};

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

  unsigned long volatile _cnt = 0;

  unsigned long ticks() const
  {
    unsigned long c;
    Irq_guard g;
    {
      c = _cnt << 8;
    }
    return c + TCNT0;
  }

  unsigned long cnt() const
  {
    Irq_guard g;
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

template<uint8_t MSK, unsigned long DELAY = 5, bool NEG = false>
struct Debounce
{
  unsigned long db_time = 0;
  bool _last:1;
  bool _old:1;
  bool _running:1;

  Debounce(uint8_t pv = PINB) : db_time(0)
  {
    _last = !!(pv & MSK);
    _old = _last;
    _running = false;
  }

  void init(uint8_t pv = PINB)
  {
    db_time = 0;
    _last = !!(pv & MSK);
    _old = _last;
    _running = false;
  }


  bool running() const { return _running; }

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

template<typename TIMER, unsigned long ON_TIME_SECS = 1800>
struct Timed_pwr_on
{
  unsigned long _pwr_on_tick;
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

  bool timeout(unsigned long now)
  {
    if (_pwr != P_timer)
      return false;

    unsigned long d = now - _pwr_on_tick;
    if (d >= ON_TIME_SECS * TIMER::Cnt_freq) {
      _pwr = P_off;
      switch_acc_off();
      return true;
    }

    return false;
  }

  void start_timer(unsigned long now)
  {
    _pwr_on_tick = now;
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

  void switch_acc_on()
  {
    PORTB |= ACC_OUT_MSK;
  }

  void switch_acc_off()
  {
    PORTB &= ~ACC_OUT_MSK;
  }

  template<typename ACC>
  bool power_btn(ACC const &acc, unsigned long now)
  {
    switch (_pwr)
    {
      case P_off:
        if (acc.state())
          _pwr = P_acc;
        else
          start_timer(now);

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
  void acc_update(ACC const &acc)
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
          _pwr = P_off;
          switch_acc_off();
          return;
      }
    }
  }
};

static Timer timer;

static void init_clk()
{
  CLKPR = 0x80;
  CLKPR = 0x03; // prescaler: 8 -> 1MHz
}	

static void init_timer()
{
  TCCR0A = 0x02; // 0x00;
  TCCR0B = 0x05; // normal, 1024 prescaler
  OCR0A  = timer.Max_tick - 1;
  TIFR = 0xff;
  TIMSK = 0x10; //0x02;
  GTCCR = 0;
}

ISR(TIMER0_COMPA_vect)
{
  ++timer._cnt;
}

static volatile uint8_t _pin_changed = false;
ISR(PCINT0_vect)
{
  _pin_changed = true;
}

static Debounce<PWR_BTN_MSK, 5, true> pwr_btn;
static Debounce<ACC_IN_MSK, 5> acc_in;

static Timed_pwr_on<Timer, 10 /*1800*/> timed_pwr;

static void do_sleep()
{
  sei();
  sleep_cpu();
  cli();
}

int main()
{
  init_clk();
  init_timer();

  set_sleep_mode(SLEEP_MODE_IDLE | _SLEEP_ENABLE_MASK);

  sei();
  DDRB  |= ACC_OUT_MSK; // ACC_OUT as output
  PORTB |= PWR_BTN_MSK; // pullup power btn pin

  // after configuring the IOs initilize the debouncers
  acc_in.init();
  pwr_btn.init();

  GIMSK = 1 << 5;
  PCMSK = ACC_IN_MSK | PWR_BTN_MSK;

  for (;;) {
    uint8_t pinb = PINB;
    Timer::Time now = timer.now();
    if (acc_in.update(now.ticks(), pinb)
        && acc_in.pressed() != 0)
      timed_pwr.acc_update(acc_in);

    if (pwr_btn.update(now.ticks(), pinb)
        && (pwr_btn.pressed() < 0)) // release
      timed_pwr.power_btn(acc_in, now.cnt);

    timed_pwr.timeout(now.cnt);

    if (pwr_btn.running() || acc_in.running())
      continue;

    {
      Irq_guard g;
      if (_pin_changed) {
        _pin_changed = false;
        continue;
      }

      if (timed_pwr.running()) {
        //do_sleep();
        _pin_changed = false;
        continue;
      }

      // sleep
      set_sleep_mode(SLEEP_MODE_PWR_DOWN | _SLEEP_ENABLE_MASK);
      //do_sleep();
      _pin_changed = false;
      set_sleep_mode(SLEEP_MODE_IDLE | _SLEEP_ENABLE_MASK);
    }
  }
}
