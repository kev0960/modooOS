#ifndef TYPE_TRAITS_H
#define TYPE_TRAITS_H

#include "types.h"

namespace Kernel {

template <class T, T v>
struct integral_constant {
  static constexpr T value = v;
  using value_type = T;
  using type = integral_constant;

  constexpr operator value_type() const noexcept { return value; }
  constexpr value_type operator()() const noexcept { return value; }
};

using true_type = integral_constant<bool, true>;
using false_type = integral_constant<bool, false>;

// remove_cv, remove_const, remove_volatile
template <typename T>
struct remove_const {
  using type = T;
};
template <typename T>
struct remove_const<const T> {
  using type = T;
};

template <typename T>
struct remove_volatile {
  using type = T;
};
template <typename T>
struct remove_volatile<volatile T> {
  using type = T;
};

template <typename T>
struct remove_cv {
  using type = typename remove_volatile<typename remove_const<T>::type>::type;
};

// is_same
template <typename T, typename U>
struct is_same : false_type {};

template <typename T>
struct is_same<T, T> : true_type {};

// enable_if (from cppreference)
template <bool B, typename T = void>
struct enable_if {};

template <typename T>
struct enable_if<true, T> {
  using type = T;
};

template <bool B, typename T = void>
using enable_if_t = typename enable_if<B, T>::type;

template <typename T>
struct _is_integral_base : false_type {};

template <>
struct _is_integral_base<bool> : true_type {};
template <>
struct _is_integral_base<char> : true_type {};
template <>
struct _is_integral_base<char16_t> : true_type {};
template <>
struct _is_integral_base<char32_t> : true_type {};
template <>
struct _is_integral_base<wchar_t> : true_type {};
template <>
struct _is_integral_base<int16_t> : true_type {};
template <>
struct _is_integral_base<int32_t> : true_type {};
template <>
struct _is_integral_base<int64_t> : true_type {};
template <>
struct _is_integral_base<uint16_t> : true_type {};
template <>
struct _is_integral_base<uint32_t> : true_type {};
template <>
struct _is_integral_base<uint64_t> : true_type {};

template <typename T>
struct is_integral : _is_integral_base<typename remove_cv<T>::type> {};
}  // namespace Kernel

#endif
