
// Licensed under the MIT License. See LICENSE.txt file in the project root.

#pragma once

#include "cxx_typetraits.h"
#include <inttypes.h>


namespace cxx {


/// return the number of bits needed to represent the given integer
template<typename T>
constexpr int bit_width(T x) noexcept
{
  return (x == 0) ? 0 : (1 + bit_width(x >> 1));
}

/// return the smallest integer type that has more or equal to the given number of bits.
template<int bits>
struct uint_for_bits
{
  template<bool COND, typename T, typename F = void>
  struct _cond;

  template<typename T, typename F>
  struct _cond<true, T, F> { using type = T; };

  template<typename T, typename F>
  struct _cond<false, T, F> { using type = F; };

  using type = typename _cond<(bits <= 8),  uint8_t,
               typename _cond<(bits <= 16), uint16_t,
               typename _cond<(bits <= 24), __uint24,
               typename _cond<(bits <= 32), uint32_t,
               uint64_t>::type>::type>::type>::type;
};

/// type alias template for uint_for_bits
template<int bits>
using uint_for_bits_t = typename uint_for_bits<bits>::type;

/// return the smallest integer type that can hold the given value
template<uint64_t val>
struct uint_for_val : uint_for_bits<bit_width(val)> {};

/// type alias template for uint_for_val
template<uint64_t val>
using uint_for_val_t = typename uint_for_val<val>::type;

template<int bits>
using int_for_bits_t = signed_type_t<uint_for_bits_t<bits>>;

template<uint64_t val>
using int_for_val_t = signed_type_t<uint_for_val_t<val>>;

}
