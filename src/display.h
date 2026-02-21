// Licensed under the MIT License. See LICENSE.txt file in the project root.

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

    static auto const PROGMEM c = I2c_master::mk_cmds(
      I2c_master::C_start,
      I2c_master::send_bytes(0x78, 0x00, 0xaf),
      I2c_master::C_stop,
      I2c_master::End(nullptr));

    I2c_master::m.start_cmds(&c);
  }
  void off()
  {
    if (!_initialized)
      return;

    static auto const PROGMEM c = I2c_master::mk_cmds(
      I2c_master::C_start,
      I2c_master::send_bytes(0x78, 0x00, 0xae),
      I2c_master::C_stop,
      I2c_master::End(nullptr));

    I2c_master::m.start_cmds(&c);
  }

  void clr()
  {
    if (!_initialized)
      return;

    static auto const PROGMEM c = I2c_master::mk_cmds(
      I2c_master::C_start,
      I2c_master::send_bytes(0x78, 0x00,
        0x20, 0x01, 0x21, 0x00, 0x7f, 0x22, 0x00, 0x03),
      I2c_master::C_stop,
      I2c_master::C_start,
      I2c_master::send_bytes(0x78, 0x40),
      I2c_master::send_bytes_rep<0>(0x0),
      I2c_master::send_bytes_rep<0>(0x0),
      I2c_master::C_stop,
      I2c_master::End(nullptr));

    I2c_master::m.start_cmds(&c);

  }

  void time(uint16_t t)
  {
    if (!_initialized)
      return;

    union BUF {
        struct {
          uint8_t s, e, idx;
        };
        RB_char_desc c;
    };
    static uint16_t ctim;
    ctim = t;
    static uint8_t xpos;
    xpos = 0x20;

    static BUF xx;
    static auto set_viewport = [](uint8_t val) {
        xx.idx = val;
        xx.s = xpos;
        xx.e = xpos + pgm_read_byte(&char_desc[val].width) - 1;
        xpos = xx.e + 2;
    };

    auto draw_char_cb = [](uint8_t){
        uint8_t idx = xx.idx;
        xx.c.addr = (uint8_t const *)pgm_read_ptr(&char_desc[idx].addr);
        xx.c.width = pgm_read_byte(&char_desc[idx].width);
        uint8_t tmp = xx.c.width << 1;
        xx.c.width += tmp;
    };

    set_viewport((ctim >> 12) & 0xf);

    static auto const PROGMEM c = I2c_master::mk_cmds(
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


      I2c_master::End(nullptr));

    I2c_master::m.start_cmds(&c);
  }

  static Display d;
};


