// Licensed under the MIT License. See LICENSE.txt file in the project root.

#pragma once

#include "cxx_ints.h"
#include <ratio>
#include <limits>

namespace cxx {

template<typename Rep>
struct duration_values
{
  static constexpr Rep zero() noexcept { return Rep(0); }
};

template<typename Rep, typename Period = std::ratio<1>>
struct duration
{
  using rep = Rep;
  using period = Period;

  Rep _c;

  duration() noexcept = default;
  constexpr duration(rep val) noexcept : _c(val) {}
  constexpr rep count() const { return _c; }

  explicit constexpr operator bool () const { return _c != 0; }

  constexpr duration(duration const &) noexcept = default;
  template<typename Rep2, typename Period2,
           typename CF = std::ratio_divide<Period2, Period>,
           typename = std::enable_if_t<CF::den == 1>>
  constexpr duration(duration<Rep2, Period2> const &d) noexcept
  : _c(static_cast<rep>(d.count() * static_cast<rep>(CF::num) / static_cast<rep>(CF::den)))
  {}

  static constexpr duration zero() noexcept { return duration(duration_values<rep>::zero()); }

  friend constexpr duration<signed_type_t<rep>, period>
  operator - (duration const &lhs, duration const &rhs) noexcept
  { return duration<signed_type_t<rep>, period>(lhs.count() - rhs.count()); }

  friend constexpr duration
  operator + (duration const &lhs, duration const &rhs) noexcept
  { return duration(lhs.count() + rhs.count()); }

  template<typename Rep2>
  friend constexpr bool
  operator >= (duration const &lhs, duration<Rep2, period> const &rhs) noexcept
  { return lhs.count() >= rhs.count(); }

  template<typename Rep2>
  friend constexpr bool
  operator > (duration const &lhs, duration<Rep2, period> const &rhs) noexcept
  { return lhs.count() > rhs.count(); }
};

template<typename To, typename Rep, typename Period>
constexpr To duration_cast(cxx::duration<Rep, Period> const &d)
{
  using to_rep = typename To::rep;
  using CF = std::ratio_divide<Period, typename To::period>;
  //using CR = long; //typename std::common_type<Rep, to_rep, uint64_t>::type;
  //using CR = typename std::common_type<Rep, to_rep>::type;
  using CR = typename std::common_type<Rep, to_rep, typename cxx::uint_for_val<CF::num * std::numeric_limits<Rep>::max()>::type>::type;
  if (CF::num == 1 && CF::den == 1)
    return To(static_cast<to_rep>(d.count()));
  if (CF::num == 1)
    //return To(static_cast<to_rep>(d.count() / static_cast<CR>(CF::den)));
    return To(static_cast<to_rep>(d.count() / static_cast<CR>(CF::den)));
  else if (CF::den == 1)
    return To(static_cast<to_rep>(d.count() * static_cast<CR>(CF::num)));
  else
    return To(static_cast<to_rep>(d.count() * static_cast<CR>(CF::num) / static_cast<CR>(CF::den)));
}

template<typename Rep, typename Period>
struct signed_type<duration<Rep, Period>> { using type = duration<signed_type_t<Rep>, Period>; };


//using microseconds = duration<uint32_t, micro>;
using milliseconds = duration<uint32_t, std::milli>;
using milliseconds16 = duration<uint16_t, std::milli>;
using seconds      = duration<__uint24>;
using seconds16    = duration<uint16_t>;
using seconds8     = duration<uint8_t>;
using minutes      = duration<__uint24, std::ratio<60>>;
using minutes8     = duration<uint8_t, std::ratio<60>>;

template<unsigned long MAX = ~0ul>
using qseconds = duration<uint_for_val_t<MAX>, std::ratio<1, 4>>;

template<unsigned BITS>
using qseconds_bits = duration<uint_for_bits_t<BITS>, std::ratio<1, 4>>;

} // namespace cxx

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wliteral-suffix"

constexpr cxx::milliseconds operator ""ms(unsigned long long ms)
{ return cxx::milliseconds(ms); }

constexpr cxx::seconds operator ""s(unsigned long long s)
{ return cxx::seconds(s); }

#pragma GCC diagnostic pop
