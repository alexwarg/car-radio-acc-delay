// Licensed under the MIT License. See LICENSE.txt file in the project root.

#pragma once

#include "i2c.h"
#include "Roboto_Condensed_24.h"

class Display
{
public:
  uint8_t _initialized:1;
  void init();
  void on()
  {
    if (!_initialized)
      return;

    static auto const c = Pgm(I2c_master::mk_cmds(
      I2c_master::C_start,
      I2c_master::send_bytes(0x78, 0x00, 0xaf),
      I2c_master::C_stop,
      I2c_master::End(nullptr)));

    I2c_master::m.start_cmds(&c);
  }
  void off()
  {
    if (!_initialized)
      return;

    static auto const c = Pgm(I2c_master::mk_cmds(
      I2c_master::C_start,
      I2c_master::send_bytes(0x78, 0x00, 0xae),
      I2c_master::C_stop,
      I2c_master::End(nullptr)));

    I2c_master::m.start_cmds(&c);
  }

  void clr()
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
      I2c_master::End(nullptr)));

    I2c_master::m.start_cmds(&c);

  }

  void time(uint16_t t)
  {
    union BUF {
        struct {
          uint8_t s, e, idx;
        };
        RB_char_desc c;
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
        xx.s = xpos;
        xx.e = xpos + pgm_read_byte(&char_desc[glyph_idx].width) - 1;
        xpos = xx.e + 1;
    };

    static auto set_viewportclr = [](uint8_t width) {
        xx.s = xpos;
        xx.e = xpos + width - 1;
        xpos = xx.e;
    };

    static auto set_viewportclr_2 = [](uint8_t) {
        set_viewportclr(2);
    };

    static auto draw_char_cb = [](uint8_t){
        uint8_t idx = xx.idx;
        xx.c.addr = (uint8_t const *)pgm_read_ptr(&char_desc[idx].addr);
        xx.c.width = pgm_read_byte(&char_desc[idx].width);
        xx.c.width *= 3;
    };

    static auto const c = Pgm(I2c_master::mk_cmds(
      I2c_master::Cb([](uint8_t) { set_viewport((ctim >> 12) & 0x0f); }),
      I2c_master::C_start,
      I2c_master::send_bytes(0x78, 0x00, 0xaf,
        0x20, 0x00, 0x21),
      I2c_master::mk_cmds(I2c_master::C_send_bytes_ram, uint8_t(2), &xx),
      I2c_master::send_bytes(0x22, 0x01, 0x03),
      I2c_master::C_stop,
      I2c_master::Cb(draw_char_cb),
      I2c_master::C_start,
      I2c_master::send_bytes(0x78, 0x40),
      I2c_master::mk_cmds(I2c_master::C_send_bytes_ram_len, &xx),
      I2c_master::C_stop,

      I2c_master::Cb(set_viewportclr_2),
      I2c_master::C_start,
      I2c_master::send_bytes(0x78, 0x00, 0x21),
      I2c_master::mk_cmds(I2c_master::C_send_bytes_ram, uint8_t(2), &xx),
      I2c_master::C_stop,
      I2c_master::C_start,
      I2c_master::send_bytes(0x78, 0x40),
      I2c_master::send_bytes_rep<6>(0x0),
      I2c_master::C_stop,

      I2c_master::Cb([](uint8_t) { set_viewport((ctim >> 8) & 0x0f); }),
      I2c_master::C_start,
      I2c_master::send_bytes(0x78, 0x00, 0x21),
      I2c_master::mk_cmds(I2c_master::C_send_bytes_ram, uint8_t(2), &xx),
      I2c_master::C_stop,
      I2c_master::Cb(draw_char_cb),
      I2c_master::C_start,
      I2c_master::send_bytes(0x78, 0x40),
      I2c_master::mk_cmds(I2c_master::C_send_bytes_ram_len, &xx),
      I2c_master::C_stop,

      I2c_master::Cb(set_viewportclr_2),
      I2c_master::C_start,
      I2c_master::send_bytes(0x78, 0x00, 0x21),
      I2c_master::mk_cmds(I2c_master::C_send_bytes_ram, uint8_t(2), &xx),
      I2c_master::C_stop,
      I2c_master::C_start,
      I2c_master::send_bytes(0x78, 0x40),
      I2c_master::send_bytes_rep<6>(0x0),
      I2c_master::C_stop,

      I2c_master::Cb([](uint8_t) { set_viewport(10); }),
      I2c_master::C_start,
      I2c_master::send_bytes(0x78, 0x00, 0x21),
      I2c_master::mk_cmds(I2c_master::C_send_bytes_ram, uint8_t(2), &xx),
      I2c_master::C_stop,
      I2c_master::Cb(draw_char_cb),
      I2c_master::C_start,
      I2c_master::send_bytes(0x78, 0x40),
      I2c_master::mk_cmds(I2c_master::C_send_bytes_ram_len, &xx),
      I2c_master::C_stop,

      I2c_master::Cb(set_viewportclr_2),
      I2c_master::C_start,
      I2c_master::send_bytes(0x78, 0x00, 0x21),
      I2c_master::mk_cmds(I2c_master::C_send_bytes_ram, uint8_t(2), &xx),
      I2c_master::C_stop,
      I2c_master::C_start,
      I2c_master::send_bytes(0x78, 0x40),
      I2c_master::send_bytes_rep<6>(0x0),
      I2c_master::C_stop,

      I2c_master::Cb([](uint8_t) { set_viewport((ctim >> 4) & 0x0f); }),
      I2c_master::C_start,
      I2c_master::send_bytes(0x78, 0x00, 0x21),
      I2c_master::mk_cmds(I2c_master::C_send_bytes_ram, uint8_t(2), &xx),
      I2c_master::C_stop,
      I2c_master::Cb(draw_char_cb),
      I2c_master::C_start,
      I2c_master::send_bytes(0x78, 0x40),
      I2c_master::mk_cmds(I2c_master::C_send_bytes_ram_len, &xx),
      I2c_master::C_stop,

      I2c_master::Cb(set_viewportclr_2),
      I2c_master::C_start,
      I2c_master::send_bytes(0x78, 0x00, 0x21),
      I2c_master::mk_cmds(I2c_master::C_send_bytes_ram, uint8_t(2), &xx),
      I2c_master::C_stop,
      I2c_master::C_start,
      I2c_master::send_bytes(0x78, 0x40),
      I2c_master::send_bytes_rep<6>(0x0),
      I2c_master::C_stop,

      I2c_master::Cb([](uint8_t) { set_viewport(ctim & 0x0f); }),
      I2c_master::C_start,
      I2c_master::send_bytes(0x78, 0x00, 0x21),
      I2c_master::mk_cmds(I2c_master::C_send_bytes_ram, uint8_t(2), &xx),
      I2c_master::C_stop,
      I2c_master::Cb(draw_char_cb),
      I2c_master::C_start,
      I2c_master::send_bytes(0x78, 0x40),
      I2c_master::mk_cmds(I2c_master::C_send_bytes_ram_len, &xx),
      I2c_master::C_stop,

      I2c_master::Cb([](uint8_t) { set_viewportclr(10); }),
      I2c_master::C_start,
      I2c_master::send_bytes(0x78, 0x00, 0x21),
      I2c_master::mk_cmds(I2c_master::C_send_bytes_ram, uint8_t(2), &xx),
      I2c_master::C_stop,
      I2c_master::C_start,
      I2c_master::send_bytes(0x78, 0x40),
      I2c_master::send_bytes_rep<30>(0x0),
      I2c_master::C_stop,

      I2c_master::End(nullptr)));

    I2c_master::m.start_cmds(&c);
  }

  static Display d;
};


