
// Licensed under the MIT License. See LICENSE.txt file in the project root.

#pragma once

#include <inttypes.h>

namespace cxx {

template<bool B, typename T = void>
struct enable_if {};

template<typename T>
struct enable_if<true, T> { using type = T; };

template<bool B, typename T = void>
using enable_if_t = typename enable_if<B, T>::type;

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

}
