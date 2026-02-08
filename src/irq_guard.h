#pragma once

#include <avr/io.h>

namespace cxx {

/**
 * Lock-guard like guard for global IRQ disable/enable on AVR
 */
class Irq_guard
{
  uint8_t f;
public:
  Irq_guard() : f(SREG) { cli(); asm volatile ("" : : : "memory"); };
  ~Irq_guard() { asm volatile ("" : : : "memory"); SREG = f; }
};

}
