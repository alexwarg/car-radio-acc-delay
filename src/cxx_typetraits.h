
// Licensed under the MIT License. See LICENSE.txt file in the project root.

#pragma once
#pragma GCC system_header

#include <inttypes.h>

namespace cxx {

template<typename T>
T declval() noexcept; // { return *reinterpret_cast<T *>(0); }

template< typename T >
struct type_identity { using type = T; };

template< typename T, T V >
struct integral_constant
{
  static T const value = V;
  typedef T value_type;
  typedef integral_constant<T, V> type;
  constexpr T operator () () const noexcept { return value; }
};

typedef integral_constant<bool, true> true_type;
typedef integral_constant<bool, false> false_type;

template< typename T > struct remove_reference;
template< typename T >
using remove_reference_t = typename remove_reference<T>::type;

template< typename T > struct identity { typedef T type; };

template< typename T1, typename T2 > struct is_same;

template< typename T > struct remove_const;
template< typename T > using remove_const_t = typename remove_const<T>::type;

template< typename T > struct remove_volatile;

template< typename T > struct remove_cv;

template< typename T > struct remove_pointer;

template< typename T > struct remove_extent;

template< typename T > struct remove_all_extents;



template< typename, typename >
struct is_same : false_type {};

template< typename T >
struct is_same<T, T> : true_type {};

template< typename T >
struct remove_reference { typedef T type; };

template< typename T >
struct remove_reference<T &> { typedef T type; };

template< typename T >
struct remove_reference<T &&> { typedef T type; };


template< typename T > struct remove_const { typedef T type; };
template< typename T > struct remove_const<T const> { typedef T type; };

template< typename T > struct remove_volatile { typedef T type; };
template< typename T > struct remove_volatile<T volatile> { typedef T type; };

template< typename T >
struct remove_cv { typedef typename remove_const<typename remove_volatile<T>::type>::type type; };

template< typename T >
using remove_cv_t = typename remove_cv<T>::type;

template< typename T >
struct remove_cvref : remove_cv<typename remove_reference<T>::type> {};

template< typename T >
using remove_cvref_t = typename remove_cvref<T>::type;

template< typename T, typename >
struct __remove_pointer_h { typedef T type; };

template< typename T, typename I >
struct __remove_pointer_h<T, I*> { typedef I type; };

template< typename  T >
struct remove_pointer : __remove_pointer_h<T, typename remove_cv<T>::type> {};

template< typename T >
using remove_pointer_t = typename remove_pointer<T>::type;


template< typename T >
struct remove_extent { typedef T type; };

template<typename T>
using remove_extent_t = typename remove_extent<T>::type;

template< typename T >
struct remove_extent<T[]> { typedef T type; };

template< typename T, unsigned long N >
struct remove_extent<T[N]> { typedef T type; };

template< typename T >
struct remove_all_extents { typedef T type; };

template< typename T >
struct remove_all_extents<T[]> { typedef typename remove_all_extents<T>::type type; };

template< typename T, unsigned long N >
struct remove_all_extents<T[N]> { typedef typename remove_all_extents<T>::type type; };

namespace detail {
template< typename T >
auto try_add_pointer(int) -> type_identity<remove_reference_t<T>*>;
template< typename T >
auto try_add_pointer(...) -> type_identity<T>;
}

template< typename T >
struct add_pointer : decltype(detail::try_add_pointer<T>(0)) {};

template< typename T >
using add_pointer_t = typename add_pointer<T>::type;

template< typename T >
inline T &&
forward(typename cxx::remove_reference<T>::type &t)
{ return static_cast<T &&>(t); }

template< typename T >
inline T &&
forward(typename cxx::remove_reference<T>::type &&t)
{ return static_cast<T &&>(t); }

template< typename T >
inline typename cxx::remove_reference<T>::type &&
move(T &t) { return static_cast<typename cxx::remove_reference<T>::type &&>(t); }

template< bool, typename T = void >
struct enable_if {};

template< typename T >
struct enable_if<true, T> { typedef T type; };

template< bool C, typename T = void >
using enable_if_t = typename enable_if<C, T>::type;

template< typename T >
struct is_void : false_type {};

template<>
struct is_void<void> : true_type {};

template< typename T >
struct is_const : false_type {};

template< typename T >
struct is_const<T const> : true_type {};

template< typename T >
struct is_volatile : false_type {};

template< typename T >
struct is_volatile<T volatile> : true_type {};

template< typename T >
struct is_pointer : false_type {};

template< typename T >
struct is_pointer<T *> : true_type {};

template< typename T >
struct is_reference : false_type {};

template< typename T >
struct is_reference<T &> : true_type {};

template< typename T >
struct is_reference<T &&> : true_type {};

template< typename T >
struct is_function : integral_constant<bool,
  !is_const<const T>::value && !is_reference<T>::value> {};

template< bool, typename, typename >
struct conditional;

template< bool C, typename T_TRUE, typename T_FALSE >
struct conditional { typedef T_TRUE type; };

template< typename T_TRUE, typename T_FALSE >
struct conditional< false, T_TRUE, T_FALSE > { typedef T_FALSE type; };

template< bool C, typename TT, typename FT>
using conditional_t = typename conditional<C, TT, FT>::type;

template<typename T>
struct is_nullptr : cxx::is_same<decltype(nullptr), remove_reference_t<T>> {};


template<typename T>
struct is_enum : integral_constant<bool, __is_enum(T)> {};

template<typename T>
struct is_polymorphic : cxx::integral_constant<bool, __is_polymorphic(T)> {};

// is_trivial
template<typename T>
struct is_trivial : integral_constant<bool, __is_trivial(T)> {};

// is_standard_layout
template<typename T>
struct is_standard_layout : integral_constant<bool, __is_standard_layout(T)> {};

// is_literal_type
template<typename T>
struct is_literal_type : integral_constant<bool, __is_literal_type(T)> {};

// is_empty
template<typename T>
struct is_empty : integral_constant<bool, __is_empty(T)> {};

#if __cplusplus > 201103L
template<typename T>
struct is_final : integral_constant<bool, __is_final(T)> {};
#endif

template< typename T > struct is_integral : false_type {};

template<> struct is_integral<bool> : true_type {};

template<> struct is_integral<char> : true_type {};
template<> struct is_integral<signed char> : true_type {};
template<> struct is_integral<unsigned char> : true_type {};
template<> struct is_integral<short> : true_type {};
template<> struct is_integral<unsigned short> : true_type {};
template<> struct is_integral<int> : true_type {};
template<> struct is_integral<unsigned int> : true_type {};
template<> struct is_integral<long> : true_type {};
template<> struct is_integral<unsigned long> : true_type {};
template<> struct is_integral<long long> : true_type {};
template<> struct is_integral<unsigned long long> : true_type {};

template< typename T, bool = is_integral<T>::value || is_enum<T>::value >
struct __is_signed_helper : integral_constant<bool, static_cast<bool>(T(-1) < T(0))> {};

template< typename T >
struct __is_signed_helper<T, false> : integral_constant<bool, false> {};

template< typename T >
struct is_signed : __is_signed_helper<T> {};


template< typename >
struct is_array : false_type {};

template< typename T >
struct is_array<T[]> : true_type {};

template< typename T, unsigned long N >
struct is_array<T[N]> : true_type {};

template< typename T, unsigned N >
constexpr unsigned array_size(T const (&)[N]) { return N; }

namespace detail {

template<class T>
auto _test_returnable(int)
  -> decltype(void(static_cast<T(*)()>(nullptr)), cxx::true_type{});

template<class>
auto _test_returnable(...)
  -> cxx::false_type;

template<class From, class To>
auto _test_implicitly_convertible(int)
  -> decltype(void(cxx::declval<void(&)(To)>()(cxx::declval<From>())), cxx::true_type{});

template<class, class>
auto _test_implicitly_convertible(...)
  -> false_type;

} // detail

template<typename From, typename To>
struct is_convertible
: cxx::integral_constant<bool,
  (   decltype(detail::_test_returnable<To>(0))::value
   && decltype(detail::_test_implicitly_convertible<From, To>(0))::value)
  || (cxx::is_void<From>::value && cxx::is_void<To>::value)>
{};

template<typename ...>
using void_t = void;

template<typename T>
struct decay
{
private:
  using U = remove_reference_t<T>;
public:
  using type = conditional_t<
    is_array<U>::value,
    remove_extent_t<U> *,
    conditional_t<is_function<U>::value,
                  add_pointer_t<U>,
                  remove_cv_t<U>
    >
  >;
};

template<typename T>
using decay_t = typename decay<T>::type;


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
