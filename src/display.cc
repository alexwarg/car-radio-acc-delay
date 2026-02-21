// Licensed under the MIT License. See LICENSE.txt file in the project root.

#include "i2c.h"
#include "display.h"

#include <avr/pgmspace.h>

void
Display::init()
{
  static auto const PROGMEM _init = I2c_master::mk_cmds(
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
    I2c_master::End([](uint8_t err) {
      if (!err)
        Display::d._initialized = 1;
      else
        Display::d._initialized = 0;
    })
  );
  I2c_master::m.start_cmds(&_init);
}

