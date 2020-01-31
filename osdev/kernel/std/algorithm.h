#ifndef ALGORITHM_H
#define ALGORITHM_H

namespace Kernel {

// Must return a when a and b are equal.
template <typename T>
constexpr const T& max(const T& a, const T& b) {
  return a >= b ? a : b;
}

template <typename T>
constexpr const T& min(const T& a, const T& b) {
  return a <= b ? a : b;
}

// Round up m / n
template <typename T, typename U>
T integer_ratio_round_up(T m, U n) {
  if (m == 0) return 1;

  return (m - 1) / n + 1;
}

namespace std {
template <typename ForwardIt, typename T>
void fill(ForwardIt first, ForwardIt last, const T& value) {
  for (; first != last; ++first) {
    *first = value;
  }
}

template <class InputIt, class T>
constexpr InputIt find(InputIt first, InputIt last, const T& v) {
  for (; first != last; ++first) {
    if (*first == v) {
      return first;
    }
  }
  return last;
}

template <class InputIt, class UnaryPredicate>
constexpr InputIt find_if(InputIt first, InputIt last, UnaryPredicate p) {
  for (; first != last; ++first) {
    if (p(*first)) {
      return first;
    }
  }
  return last;
}

}  // namespace std
}  // namespace Kernel

#endif
