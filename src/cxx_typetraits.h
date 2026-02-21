
// Licensed under the MIT License. See LICENSE.txt file in the project root.

#pragma once

namespace cxx {

template<bool B, typename T = void>
struct enable_if {};

template<typename T>
struct enable_if<true, T> { using type = T; };

template<bool B, typename T = void>
using enable_if_t = typename enable_if<B, T>::type;

template<typename T>
constexpr int bit_width(T x) noexcept
{
  return (x == 0) ? 0 : (1 + bit_width(x >> 1));
}

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

template<int bits>
using uint_for_bits_t = typename uint_for_bits<bits>::type;

template<uint64_t val>
struct uint_for_val : uint_for_bits<bit_width(val)> {};

template<uint64_t val>
using uint_for_val_t = typename uint_for_val<val>::type;

template<typename T>
T declval() noexcept;

template<typename ...T> struct common_type;

template<> struct common_type<> {};
template<typename T> struct common_type<T> { using type = T; };
template<typename T1, typename T2>
struct common_type<T1, T2>
{ using type = decltype(false ? declval<T1>() : declval<T2>()); };

template<typename T1, typename T2, typename ...R>
struct common_type<T1, T2, R...> : common_type<typename common_type<T1, T2>::type, R...> {};
#if 0
template<typename T1, typename T2>
struct common_type
{
  using type = typename uint_for_bits< (sizeof(T1) > sizeof(T2)) ? (sizeof(T1) * 8) : (sizeof(T2) * 8) >::type;
};
#endif

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

template<int bits>
using int_for_bits_t = signed_type_t<uint_for_bits_t<bits>>;

template<uint64_t val>
using int_for_val_t = signed_type_t<uint_for_val_t<val>>;

}
