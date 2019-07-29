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
}  // namespace Kernel

#endif
