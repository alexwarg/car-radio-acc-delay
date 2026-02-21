
// Licensed under the MIT License. See LICENSE.txt file in the project root.

#pragma once

#include <type_traits>
#include <inttypes.h>

namespace cxx {

using namespace cxx_a;

template<typename T> struct signed_type;
template<> struct signed_type<uint8_t> { using type = int8_t; };
template<> struct signed_type<int8_t> { using type = int8_t; };
template<> struct signed_type<uint16_t> { using type = int16_t; };
template<> struct signed_type<int16_t> { using type = int16_t; };
template<> struct signed_type<__uint24> { using type = __int24; };
template<> struct signed_type<__int24> { using type = __int24; };
template<> struct signed_type<uint32_t> { using type = int32_t; };
template<> struct signed_type<int32_t> { using type = int32_t; };

template<typename T> using signed_type_t = typename signed_type<T>::type;

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

#ifdef __UINT64_TYPE__
  using type = typename _cond<(bits <= 8),  uint8_t,
               typename _cond<(bits <= 16), uint16_t,
               typename _cond<(bits <= 24), __uint24,
               typename _cond<(bits <= 32), uint32_t,
               uint64_t>::type>::type>::type>::type;
#else
  using type = typename _cond<(bits <= 8),  uint8_t,
               typename _cond<(bits <= 16), uint16_t,
               typename _cond<(bits <= 24), __uint24,
               uint32_t>::type>::type>::type;
#endif
};

/// type alias template for uint_for_bits
template<int bits>
using uint_for_bits_t = typename uint_for_bits<bits>::type;

/// return the smallest integer type that can hold the given value
template<__INTMAX_TYPE__ val>
struct uint_for_val : uint_for_bits<bit_width(val)> {};

/// type alias template for uint_for_val
template<__INTMAX_TYPE__ val>
using uint_for_val_t = typename uint_for_val<val>::type;

template<int bits>
using int_for_bits_t = signed_type_t<uint_for_bits_t<bits>>;

template<__UINTMAX_TYPE__ val>
using int_for_val_t = signed_type_t<uint_for_val_t<val>>;

}
