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

template <typename ForwardIt, typename T>
constexpr ForwardIt lower_bound(ForwardIt first, ForwardIt last,
                                const T& value) {
  ForwardIt it = first;
  typename ForwardIt::difference_type step, count = last - first;

  while (count > 0) {
    it = first;
    step = count / 2;
    it += step;
    if (*it < value) {
      first = ++it;
      count -= step + 1;
    } else
      count = step;
  }
  return first;
}

template <typename ForwardIt, typename T>
constexpr ForwardIt upper_bound(ForwardIt first, ForwardIt last,
                                const T& value) {
  ForwardIt it = first;
  typename ForwardIt::difference_type count, step;
  count = last - first;

  while (count > 0) {
    it = first;
    step = count / 2;
    it += step;
    if (!(value < *it)) {
      first = ++it;
      count -= step + 1;
    } else
      count = step;
  }
  return first;
}

}  // namespace std
}  // namespace Kernel

#endif
