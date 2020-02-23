#ifndef KERNEL_UTIL_H
#define KERNEL_UTIL_H

#include "../std/types.h"

#define ASSERT(cond) \
  Kernel::__::AssertTrue(cond, __PRETTY_FUNCTION__, __LINE__);

#define PANIC() Kernel::__::Panic(__PRETTY_FUNCTION__, __LINE__);

namespace Kernel {

template <typename T, typename U>
constexpr int OffsetOf(const T& t, U T::*u) {
  return (const char*)&(t.*u) - (const char*)&t;
}

// Functions below should not be directly used.
namespace __ {

void AssertTrue(bool condition, const char* func_name, int line);
void Panic(const char* func_name, int line);

}  // namespace __
}  // namespace Kernel

#endif
