// Licensed under the MIT License. See LICENSE.txt file in the project root.

#pragma once

#include <avr/io.h>

#include <stddef.h>
#include <avr/pgmspace.h>

#include "cxx_pgm.h"

#ifndef SDA_BIT
#if defined(__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
enum {
    SDA_BIT = 0,
    SCL_BIT = 2,
};
#define SDA_BIT SDA_BIT
#define SCL_BIT SCL_BIT
#define SDA_PIN PINB
#define SCL_PIN PINB
#define SDA_PORT PORTB
#define SCL_PORT PORTB
#define SDA_DDR DDRB
#define SCL_DDR DDRB
#endif
#endif

struct I2c_master
{
  enum Cmd : uint8_t
  {
    C_end = 0,
    C_callback,
    C_start,
    C_stop,
    C_send_bytes,
    C_send_bytes_inline,
    C_send_bytes_rep,
    C_send_bytes_rep_len,
    C_send_bytes_ram_len,
  };

  enum : uint8_t
  {
    M_SCL = 1 << SCL_BIT,
    M_SDA = 1 << SDA_BIT,
  };

  typedef void (*Finalizer)(uint8_t err);
  using Start_ptr = Pgm_ptr<void const>;

  struct Send_buffer_data
  {
    Gen_ptr<void const> addr;
    uint8_t len;
  } __attribute__((packed));

  struct Repeat_byte_data
  {
    uint8_t len;
    uint8_t byte;
  } __attribute__((packed));

  static I2c_master m;

private:
  //uint8_t _state;
  typedef void (*resume_fn)();
  resume_fn resume;

  using Cmd_ptr = Pgm_ptr<uint8_t const>;

  Cmd_ptr cmd;

  uint8_t len;
  union {
    Gen_ptr<uint8_t const> buf;
    uint8_t buf_byte;
  };

  static uint8_t _scl() __attribute__((always_inline))  { return SCL_PIN & M_SCL; }
  static void _scl_l() __attribute__((always_inline))  { SCL_PORT &= ~M_SCL; }
  static void _scl_h() __attribute__((always_inline))  { SCL_PORT |= M_SCL; }

  static void _sda_l() __attribute__((always_inline))  { SDA_PORT &= ~M_SDA; }
  static void _sda_h() __attribute__((always_inline))  { SDA_PORT |= M_SDA; }

  static void _sda_in() __attribute__((always_inline))  { SDA_DDR &= ~M_SDA; }
  static void _sda_out() __attribute__((always_inline))  { SDA_DDR |= M_SDA; }

  void _resume_err(uint8_t err = 1) noexcept
  {
    len = err;
    resume = _next_cmd;
  }

  void _resume_next() noexcept
  {
    resume = _next_cmd;
  }

  void _stop()
  {
    if (_scl())
      return _resume_next();

    _sda_l(); _sda_out();
    _scl_h();
    resume = []() {
      if (!m._scl())
        return;

      m.resume = []() {
        USIDR = 0xff;
        m._sda_in(); // -> SDA released: high Z
        //PORTB &= ~ACC_OUT_MSK;
        return m._resume_next();
      };
    };
  }

  void _start()
  {
    // generate start condition
    _sda_in(); _sda_l(); // results in SDA = released high Z
    _scl_h();

    resume = []() {
      if (!m._scl())
        return;

      m._sda_out(); // LOW
      //m.resume = []() {
        m.resume = []() {
          m._scl_l();
          m._sda_h();
          //PORTB |= ACC_OUT_MSK;
          _next_cmd();
        };
      //};
    };
  }

  static void _usitc() __attribute__((always_inline))
  { USICR |= (1 << USITC); }

  static uint8_t _wait_scl() noexcept
  {
    uint8_t r;
    for (uint8_t x = 0; !(r = _scl()) && (x < 20); ++x)
      ;

    return r;
  }


  template<bool STRETCH, resume_fn fn>
  static void _send_bits_loop()
  {
    if (!_wait_scl())
      {
        if (STRETCH)
          return;
        else
          return m._resume_err();
      }

    for (;;)
      {
        m._usitc();
        if (USISR & (1 << USIOIF))
          break;

        m._usitc();
        if (!_wait_scl())
          return m._resume_err();
      }

    fn();
  }

  template<uint8_t (get_dr)()>
  static uint8_t _send_byte()
  {
    USISR = 0b11110000;
    USIDR = get_dr();
    m.len--;
    m._usitc();
    m.resume = _send_bits_loop<false, [](){
      USISR = 0b11111110;
      m._sda_in();
      m._usitc();
      /*m.resume = */
      _send_bits_loop<true, []() {
        uint8_t nack = USIDR;
        m._sda_out();
        if (nack & 1)
          {
            m.len = 1;
            m._stop();
          }
        else if (m.len)
          _send_byte<get_dr>();
        else
          _next_cmd();
      }>();
    }>;
    return 1;
  }

  template<uint8_t (F)()> inline
  static void _send_bytes_x(Gen_ptr<void const> ptr, uint8_t len)
  {
    m.buf = gen_ptr_cast<uint8_t const>(ptr);
    m.len = len;
    _send_byte<F>();
  }

  static void _next_cmd()
  {
    static auto next_buf_byte = []() { return *(m.buf++); };

    switch (*m.cmd++) {
      case C_end:
        m.resume = nullptr;
          {
            Finalizer h = gen_ptr_recast<Finalizer const>(m.cmd)[0];
            m.cmd = nullptr;
            if (h)
              h(m.len);
          }
        return;

      case C_callback:
        m.resume = _next_cmd;
        m.cmd += sizeof(void*);
          {
            Finalizer h = gen_ptr_recast<Finalizer const>(m.cmd)[-1];
            if (h)
              h(m.len);
          }
        return;

      case C_start:
        if (!m.len)
          m._start();
        return;

      case C_stop:
        if (!m.len)
          m._stop();
        return;

      case C_send_bytes:
        m.cmd += 1 + sizeof(Gen_ptr<uint8_t const>);
        if (m.len)
          return;

        m._send_bytes_x<next_buf_byte>(
            gen_ptr_recast<Gen_ptr<uint8_t const>>(m.cmd)[-1],
            m.cmd[-1 - sizeof(Gen_ptr<uint8_t const>)]);
        return;

      case C_send_bytes_inline:
          {
            uint8_t l = *m.cmd;
            m.cmd += l + 1;
            if (m.len)
              return;

            m._send_bytes_x<next_buf_byte>(m.cmd - l, l);
          }

        return;

      case C_send_bytes_rep:
        m.cmd += 2;
        if (!m.len)
          {
            m.len = m.cmd[-2];
            m.buf_byte = m.cmd[-1];
            m._send_byte<[]() { return m.buf_byte; }>();
          }
        return;

      case C_send_bytes_rep_len:
        m.cmd += sizeof(Repeat_byte_data const *);
        if (!m.len)
          {
            auto s = gen_ptr_recast<Repeat_byte_data const *>(m.cmd)[-1];
            m.buf_byte = s->byte;
            m.len = s->len;
            m._send_byte<[]() { return m.buf_byte; }>();
          }
        return;

      case C_send_bytes_ram_len:
        m.cmd += sizeof(Send_buffer_data const *);
        if (!m.len)
          {
            auto s = gen_ptr_recast<Send_buffer_data const *>(m.cmd)[-1];
            m._send_bytes_x<next_buf_byte>(s->addr, s->len);
          }

        return;
    }
  }

public:
  struct End
  {
    uint8_t cmd;
    Finalizer f;
    explicit constexpr End(Finalizer f) : cmd(C_end), f(f) {}
  } __attribute__((packed));

  struct Cb
  {
    uint8_t cmd;
    Finalizer f;
    explicit constexpr Cb(Finalizer f) : cmd(C_callback), f(f) {}
  } __attribute__((packed));

  struct Send_bytes
  {
    uint8_t cmd;
    uint8_t len;
    Gen_ptr<void const> buf;

    template<typename T>
      constexpr Send_bytes(T const &b) noexcept
      : cmd(C_send_bytes), len(sizeof(b)), buf(ram_ptr(&b))
        {}

    template<typename T>
      constexpr Send_bytes(Pgm<T> const &b) noexcept
      : cmd(C_send_bytes), len(sizeof(b)), buf(&b)
        {}
  } __attribute__((packed));

  struct Send_buffer
  {
    uint8_t cmd;
    Send_buffer_data *d;

    constexpr Send_buffer(Send_buffer_data *ptr) noexcept
      : cmd(C_send_bytes_ram_len), d(ptr)
        {}
  } __attribute__((packed));

  template<typename ...T>
  struct Packed_tuple;

  template<typename T>
  struct Packed_tuple<T>
  {
    T v;
  } __attribute__((packed));

  template<typename T, typename ...R>
  struct Packed_tuple<T, R...>
  {
    T v;
    Packed_tuple<R...> r;
  } __attribute__((packed));

  template<typename ...T>
  static constexpr Packed_tuple<T...> mk_cmds(T &&...args)
  {
    return Packed_tuple<T...>{args...};
  }

  template<typename T, unsigned ext>
  struct Array
  {
    T x[ext];
  };

  template<typename ...T>
  static constexpr Array<uint8_t, sizeof...(T) + 2>
  send_bytes(T &&...bytes) noexcept
  {
    return Array<uint8_t, sizeof...(T) + 2>{C_send_bytes_inline, uint8_t(sizeof...(T)), uint8_t(bytes)...};
  }

  template<unsigned N>
  static constexpr Array<uint8_t, 3>
  send_bytes_rep(uint8_t bytes) noexcept
  {
    return { C_send_bytes_rep, N, bytes };
  }

  static constexpr Array<uint8_t, 3>
  send_bytes_rep(uint8_t byte, uint8_t len) noexcept
  {
    return { C_send_bytes_rep, len, byte };
  }

  static constexpr auto
  send_bytes_rep_len(Repeat_byte_data const *p) noexcept
  {
    return mk_cmds((uint8_t)C_send_bytes_rep_len, (Repeat_byte_data const *)p);
  }

  static void init()
  {
    SDA_DDR  &= ~M_SDA;
    SDA_PORT &= ~M_SDA;
    //USIDR = 0xFF;

    USICR = (1 << USIWM1) | (1 << USICS1) | (1 << USICLK);
    USISR = (1 << USISIF) | (1 << USIOIF) | (1 << USIPF) |
            (1 << USIDC) |    // Clear flags,
            (0x0 << USICNT0); // and reset counter.

    // set SCL as output HiZ
    SCL_DDR  |= M_SCL;
    SCL_PORT |= M_SCL;
  }

  void start_cmds(Pgm_ptr<void const> cmds)
  {
    len = 0;
    cmd = gen_ptr_cast<uint8_t const>(cmds);
    resume = _next_cmd;
  }

  void step()
  {
    if (resume)
      resume();
  }

  bool busy() const noexcept { return resume != nullptr; }

  bool might_sleep() const { return !resume; }
  bool might_power_down() const { return !resume; }
};


