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

template <typename T, typename U>
class pair {
 public:
  pair(const T& first, const U& second) : first(first), second(second) {}
  pair(T&& first, U&& second)
      : first(std::move(first)), second(std::move(second)) {}
  pair(const pair& p) : first(p.first), second(p.second) {}
  pair(pair&& p) : first(std::move(p.first)), second(std::move(p.second)) {}

  T first;
  U second;
};

template <typename T, typename U>
std::pair<T, U> make_pair(T t, U u) {
  return std::pair<T, U>(t, u);
}

}  // namespace std
}  // namespace Kernel
#endif
