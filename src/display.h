// Licensed under the MIT License. See LICENSE.txt file in the project root.

#pragma once

#include "i2c.h"
#include "Roboto_Condensed_24.h"

class Display
{
public:
  uint8_t _initialized:1;
  uint8_t _on:1;
  void init();

  template<I2c_master::Finalizer finished = nullptr,
    typename STARTER = void (*)(I2c_master::Start_ptr)>
  void on(STARTER &&starter = [](I2c_master::Start_ptr c) { I2c_master::m.start_cmds(c); })
  {
    if (!_initialized)
      return;

    static auto const c = Pgm(I2c_master::mk_cmds(
      I2c_master::C_start,
      I2c_master::send_bytes(0x78, 0x00, 0xaf),
      I2c_master::C_stop,
      I2c_master::End(finished)));

    starter(&c);
  }

  template<I2c_master::Finalizer finished = nullptr,
    typename STARTER = void (*)(I2c_master::Start_ptr)>
  void off(STARTER &&starter = [](I2c_master::Start_ptr c) { I2c_master::m.start_cmds(c); })
  {
    if (!_initialized)
      return;

    static auto const c = Pgm(I2c_master::mk_cmds(
      I2c_master::C_start,
      I2c_master::send_bytes(0x78, 0x00, 0xae),
      I2c_master::C_stop,
      I2c_master::End(finished)));

    starter(&c);
  }

  template<I2c_master::Finalizer finished = nullptr,
    typename STARTER = void (*)(I2c_master::Start_ptr)>
  void clr(STARTER &&starter = [](I2c_master::Start_ptr c) { I2c_master::m.start_cmds(c); })
  {
    if (!_initialized)
      return;

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
      I2c_master::End(finished)));

    starter(&c);

  }

  template<I2c_master::Finalizer finished = nullptr,
    typename STARTER = void (*)(I2c_master::Start_ptr)>
  void time(uint16_t t, STARTER &&starter = [](I2c_master::Start_ptr c) { I2c_master::m.start_cmds(c); })
  {
    union BUF
    {
      struct
      {
        struct
        {
          uint8_t s, e;
        } v;
        uint8_t idx;
      };
      I2c_master::Send_buffer_data c;
    };
    static uint16_t ctim;
    static uint8_t xpos;
    static BUF xx;

    if (!_initialized)
      return;

    if (t == ctim)
      return;

    ctim = t;
    xpos = 32;

    static auto set_viewport = [](uint8_t glyph_idx) {
        xx.idx = glyph_idx;
        xx.v.s = xpos;
        xx.v.e = xpos + pgm_read_byte(&char_desc[glyph_idx].width) - 1;
        xpos = xx.v.e + 1;
    };

    static auto set_viewportclr = [](uint8_t width) {
        xx.v.s = xpos;
        xx.v.e = xpos + width - 1;
        xpos = xx.v.e;
    };

    static auto set_viewportclr_2 = [](uint8_t) {
        set_viewportclr(2);
    };

    static auto draw_char_cb = [](uint8_t){
        uint8_t idx = xx.idx;
        xx.c.addr = pgm_ptr(pgm_read_ptr(&char_desc[idx].addr));
        xx.c.len = pgm_read_byte(&char_desc[idx].width);
        xx.c.len *= 3;
    };

    static auto const c = Pgm(I2c_master::mk_cmds(
      I2c_master::Cb([](uint8_t) { set_viewport((ctim >> 12) & 0x0f); }),
      I2c_master::C_start,
      I2c_master::send_bytes(0x78, 0x00, 0xaf,
        0x20, 0x00, 0x21),
      I2c_master::Send_bytes(xx.v),
      I2c_master::send_bytes(0x22, 0x01, 0x03),
      I2c_master::C_stop,
      I2c_master::Cb(draw_char_cb),
      I2c_master::C_start,
      I2c_master::send_bytes(0x78, 0x40),
      I2c_master::Send_buffer(&xx.c),
      I2c_master::C_stop,

      I2c_master::Cb(set_viewportclr_2),
      I2c_master::C_start,
      I2c_master::send_bytes(0x78, 0x00, 0x21),
      I2c_master::Send_bytes(xx.v),
      I2c_master::C_stop,
      I2c_master::C_start,
      I2c_master::send_bytes(0x78, 0x40),
      I2c_master::send_bytes_rep<6>(0x0),
      I2c_master::C_stop,

      I2c_master::Cb([](uint8_t) { set_viewport((ctim >> 8) & 0x0f); }),
      I2c_master::C_start,
      I2c_master::send_bytes(0x78, 0x00, 0x21),
      I2c_master::Send_bytes(xx.v),
      I2c_master::C_stop,
      I2c_master::Cb(draw_char_cb),
      I2c_master::C_start,
      I2c_master::send_bytes(0x78, 0x40),
      I2c_master::Send_buffer(&xx.c),
      I2c_master::C_stop,

      I2c_master::Cb(set_viewportclr_2),
      I2c_master::C_start,
      I2c_master::send_bytes(0x78, 0x00, 0x21),
      I2c_master::Send_bytes(xx.v),
      I2c_master::C_stop,
      I2c_master::C_start,
      I2c_master::send_bytes(0x78, 0x40),
      I2c_master::send_bytes_rep<6>(0x0),
      I2c_master::C_stop,

      I2c_master::Cb([](uint8_t) { set_viewport(10); }),
      I2c_master::C_start,
      I2c_master::send_bytes(0x78, 0x00, 0x21),
      I2c_master::Send_bytes(xx.v),
      I2c_master::C_stop,
      I2c_master::Cb(draw_char_cb),
      I2c_master::C_start,
      I2c_master::send_bytes(0x78, 0x40),
      I2c_master::Send_buffer(&xx.c),
      I2c_master::C_stop,

      I2c_master::Cb(set_viewportclr_2),
      I2c_master::C_start,
      I2c_master::send_bytes(0x78, 0x00, 0x21),
      I2c_master::Send_bytes(xx.v),
      I2c_master::C_stop,
      I2c_master::C_start,
      I2c_master::send_bytes(0x78, 0x40),
      I2c_master::send_bytes_rep<6>(0x0),
      I2c_master::C_stop,

      I2c_master::Cb([](uint8_t) { set_viewport((ctim >> 4) & 0x0f); }),
      I2c_master::C_start,
      I2c_master::send_bytes(0x78, 0x00, 0x21),
      I2c_master::Send_bytes(xx.v),
      I2c_master::C_stop,
      I2c_master::Cb(draw_char_cb),
      I2c_master::C_start,
      I2c_master::send_bytes(0x78, 0x40),
      I2c_master::Send_buffer(&xx.c),
      I2c_master::C_stop,

      I2c_master::Cb(set_viewportclr_2),
      I2c_master::C_start,
      I2c_master::send_bytes(0x78, 0x00, 0x21),
      I2c_master::Send_bytes(xx.v),
      I2c_master::C_stop,
      I2c_master::C_start,
      I2c_master::send_bytes(0x78, 0x40),
      I2c_master::send_bytes_rep<6>(0x0),
      I2c_master::C_stop,

      I2c_master::Cb([](uint8_t) { set_viewport(ctim & 0x0f); }),
      I2c_master::C_start,
      I2c_master::send_bytes(0x78, 0x00, 0x21),
      I2c_master::Send_bytes(xx.v),
      I2c_master::C_stop,
      I2c_master::Cb(draw_char_cb),
      I2c_master::C_start,
      I2c_master::send_bytes(0x78, 0x40),
      I2c_master::Send_buffer(&xx.c),
      I2c_master::C_stop,

      I2c_master::Cb([](uint8_t) { set_viewportclr(10); }),
      I2c_master::C_start,
      I2c_master::send_bytes(0x78, 0x00, 0x21),
      I2c_master::Send_bytes(xx.v),
      I2c_master::C_stop,
      I2c_master::C_start,
      I2c_master::send_bytes(0x78, 0x40),
      I2c_master::send_bytes_rep<30>(0x0),
      I2c_master::C_stop,

      I2c_master::End(finished)));

    starter(&c);
  }

  static Display d;
};


