#ifndef UTILITY_H
#define UTILITY_H

#include "type_traits.h"

namespace Kernel {
namespace std {

// Blatantly copied from
// https://stackoverflow.com/questions/27501400/the-implementation-of-stdforward
template <typename T>
constexpr T&& forward(typename remove_reference<T>::type& t) noexcept {
  return static_cast<T&&>(t);
}

template <typename T>
constexpr T&& forward(typename remove_reference<T>::type&& t) noexcept {
  static_assert(!std::is_lvalue_reference<T>::value,
                "Can not forward an rvalue as an lvalue.");
  return static_cast<T&&>(t);
}

template <typename T>
typename remove_reference<T>::type&& move(T&& arg) {
  return static_cast<typename remove_reference<T>::type&&>(arg);
}

}  // namespace std
}  // namespace Kernel
#endif
