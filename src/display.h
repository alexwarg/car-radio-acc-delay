// Licensed under the MIT License. See LICENSE.txt file in the project root.

#pragma once

#include "i2c.h"
#include "fonts/Roboto_Condensed_24.h"
#include "fonts/DSEG7_Modern-Italic_24.h"
#include <utility>

class Display
{
public:
  uint8_t _initialized:1;
  uint8_t _on:1;

  template<auto finish>
  void init()
  {
    static auto const _init = Pgm(I2c_master::mk_cmds(
      // init seq
      I2c_master::C_start,
      I2c_master::send_bytes(
        0x78, 0x00, 0xae, 0xd5, 0x80, 0xa8, 0x1f,
        0xd3, 0x00, 0x40, 0x8d, 0x14,
        0x20, 0x00, 0xa1, 0xc8,
        0xda, 0x02, 0x81, 0x8f,
        0xd9, 0xf1,
        0xdb, 0x40, 0xa4, 0xa6, 0x2e),
      I2c_master::C_stop,

      // clear screen
      I2c_master::C_start,
      I2c_master::send_bytes(0x78, 0x00,
        0x20, 0x01, 0x21, 0x00, 0x7f, 0x22, 0x00, 0x03),
      I2c_master::C_stop,
      I2c_master::C_start,
      I2c_master::send_bytes(0x78, 0x40),
      I2c_master::send_bytes_rep<0>(0x0),
      I2c_master::send_bytes_rep<0>(0x0),
      I2c_master::C_stop,

      // done
      I2c_master::End(finish)
    ));
    I2c_master::m.start_cmds(&_init);
  }


  template<I2c_master::Finalizer finished,
    typename STARTER = void (*)(I2c_master::Start_ptr)>
  void on(STARTER &&starter = [](I2c_master::Start_ptr c) { I2c_master::m.start_cmds(c); })
  {
    static auto const c = Pgm(I2c_master::mk_cmds(
      I2c_master::C_start,
      I2c_master::send_bytes(0x78, 0x00, 0xaf),
      I2c_master::C_stop,
      I2c_master::End(finished)));

    starter(&c);
  }

  template<I2c_master::Finalizer finished,
    typename STARTER = void (*)(I2c_master::Start_ptr)>
  void off(STARTER &&starter = [](I2c_master::Start_ptr c) { I2c_master::m.start_cmds(c); })
  {
    static auto const c = Pgm(I2c_master::mk_cmds(
      I2c_master::C_start,
      I2c_master::send_bytes(0x78, 0x00, 0xae),
      I2c_master::C_stop,
      I2c_master::End(finished)));

    starter(&c);
  }

  template<I2c_master::Finalizer finished,
    typename STARTER = void (*)(I2c_master::Start_ptr)>
  void clr(STARTER &&starter = [](I2c_master::Start_ptr c) { I2c_master::m.start_cmds(c); })
  {
    static auto const c = Pgm(I2c_master::mk_cmds(
      I2c_master::C_start,
      I2c_master::send_bytes(0x78, 0x00,
        0x20, 0x01, 0x21, 0x00, 0x7f, 0x22, 0x00, 0x03),
      I2c_master::C_stop,
      I2c_master::C_start,
      I2c_master::send_bytes(0x78, 0x40),
      I2c_master::send_bytes_rep<0>(0x0),
      I2c_master::send_bytes_rep<0>(0x0),
      I2c_master::C_stop,
      I2c_master::C_start,
      I2c_master::send_bytes(0x78, 0x00, 0x20, 0x00),
      I2c_master::C_stop,
      I2c_master::End(finished)));

    starter(&c);

  }

  template<typename ...EXTRA>
  constexpr static auto
  mk_clr(uint8_t x, uint8_t y, uint8_t w, uint8_t h, EXTRA &&...e)
  {
    return I2c_master::mk_cmds(
        I2c_master::C_start,
        I2c_master::send_bytes(0x78, 0x00, e...,
          0x21, x, x + w - 1, 0x22, y, y + h -1),
        I2c_master::C_stop,
        I2c_master::C_start,
        I2c_master::send_bytes(0x78, 0x40),
        I2c_master::send_bytes_rep(0, h * w),
        I2c_master::C_stop);
  }

  template<typename T, typename ...EXTRA>
  constexpr static auto
  mk_blit(uint8_t x, uint8_t y, uint8_t w, uint8_t h, Pgm<T> const &img, EXTRA &&...e)
  {
    return I2c_master::mk_cmds(
        I2c_master::C_start,
        I2c_master::send_bytes(0x78, 0x00, e...,
          0x21, x, x + w - 1, 0x22, y, y + h -1),
        I2c_master::C_stop,
        I2c_master::C_start,
        I2c_master::send_bytes(0x78, 0x40),
        I2c_master::Send_bytes(img),
        I2c_master::C_stop);
  }

  template<typename T, typename ...EXTRA>
  constexpr static auto
  mk_blit(uint8_t x, uint8_t y, EXTRA &&...e)
  {
    return mk_blit(x, y, T::w, T::h / 8, T::data, std::forward<EXTRA>(e)...);
  }

  struct Disp_time
  {
    struct Range { uint8_t s, e; };
    struct Range_n_idx { Range v; uint8_t idx; };
    struct State
    {
      uint16_t ctim;
      uint8_t xpos;
      uint8_t pos;
      union
      {
        Range_n_idx ri;
        I2c_master::Send_buffer_data c;
        I2c_master::Repeat_byte_data rb;
      };
    };

    static State state;

    using Fnt = Fnt_dseg7_modern_italic_24;
    //using Fnt = Fnt_roboto_condensed_24;

    static uint8_t get_idx()
    {
      if (state.pos < 2)
        return (state.ctim >> (12 - (state.pos * 4))) & 0x0f;
      else if (state.pos == 2)
        return 10;
      else
        return (state.ctim >> (12 - ((state.pos - 1) * 4))) & 0x0f;
    }

    static void set_viewport(uint8_t, auto)
    {
      state.ri.idx = get_idx();
      state.ri.v.s = state.xpos;
      state.ri.v.e = state.xpos + Fnt::width[state.ri.idx] - 1;
      state.xpos = state.ri.v.e + 1;
    }

    static void set_viewportclr(uint8_t width)
    {
      state.ri.v.s = state.xpos;
      state.ri.v.e = state.xpos + width - 1;
      state.xpos = state.ri.v.e;
    }

    static void set_viewportclr_2(uint8_t)
    {
      set_viewportclr(state.pos == 4 ? 10 : 2);
    }

    static void clr_cb(uint8_t)
    {
      if (state.pos == 4)
        state.rb.len = 30;
      else
        state.rb.len = 6;

      state.rb.byte = 0;
    }

    static void pre_clr(uint8_t, auto)
    {
      state.rb.len = Fnt::xoff[state.ri.idx] * 3;
      state.rb.byte = 0;
    }

    static void post_clr(uint8_t, auto)
    {
      auto const idx = get_idx();
      state.rb.len = (Fnt::width[idx] - Fnt::xoff[idx] - Fnt::cwidth[idx]) * 3;
      state.rb.byte = 0;
    }

    static void draw_char_cb(uint8_t, auto)
    {
      uint8_t idx = state.ri.idx;
      state.c.addr = Fnt::charsx[idx];
      state.c.len = Fnt::cwidth[idx] * 3;
    }
  };

  template<I2c_master::Finalizer finished,
    typename STARTER = void (*)(I2c_master::Start_ptr)>
  void time(uint16_t t, bool force,
            STARTER &&starter = [](I2c_master::Start_ptr c) { I2c_master::m.start_cmds(c); })
  {
    if (!force && t == Disp_time::state.ctim)
      return;

    Disp_time::state.ctim = t;
    Disp_time::state.xpos = 40;
    Disp_time::state.pos = 0;

    static auto const r = Pgm(
      I2c_master::do_while_end<
        [](){return Display::Disp_time::state.pos++ < 4;}, finished>(
        I2c_master::mk_cmds(
          I2c_master::Cb(Display::Disp_time::set_viewport),
          I2c_master::C_start,
          I2c_master::send_bytes(0x78, 0x00, 0xaf, 0x21),
          I2c_master::Send_bytes(Display::Disp_time::state.ri.v),
          I2c_master::send_bytes(0x22, 0x01, 0x03),
          I2c_master::C_stop,
          I2c_master::Cb(Display::Disp_time::pre_clr),
          I2c_master::C_start,
          I2c_master::send_bytes(0x78, 0x40),
          I2c_master::send_bytes_rep_len(&Display::Disp_time::state.rb),
          I2c_master::Cb(Display::Disp_time::draw_char_cb),
          I2c_master::Send_buffer(&Display::Disp_time::state.c),
          I2c_master::Cb(Display::Disp_time::post_clr),
          I2c_master::send_bytes_rep_len(&Display::Disp_time::state.rb),
          I2c_master::C_stop
        )
      )
    );

    starter(&r);
  }

  static Display d;
};

