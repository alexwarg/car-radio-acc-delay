// Licensed under the MIT License. See LICENSE.txt file in the project root.

#pragma once

#include "cxx_typetraits.h"

namespace cxx {

template<typename M, typename N>
constexpr typename common_type<M, N>::type
gcd(M m, N n)
{
  if (n == 0)
    return m;

  using T = typename common_type<M, N>::type;

  T rem = static_cast<T>(m) % static_cast<T>(n);
  return gcd(n, rem);
}


template<uint32_t Num, uint32_t Denom = 1>
struct ratio
{
  static constexpr uint32_t num = Num / gcd(Num, Denom);
  static constexpr uint32_t den = Denom / gcd(Num, Denom);
  using type = ratio<num, den>;
};

template<typename R1, typename R2>
using ratio_divide = typename cxx::ratio<R1::num * R2::den, R1::den * R2::num>::type;

using micro = ratio<1, 1000000>;
using milli = ratio<1, 1000>;

}
