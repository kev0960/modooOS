#ifndef KERNEL_UTIL_H
#define KERNEL_UTIL_H

#define ASSERT(cond) \
  Kernel::__::AssertTrue(cond, __PRETTY_FUNCTION__, __LINE__);

#define PANIC() Kernel::__::Panic(__PRETTY_FUNCTION__, __LINE__);

namespace Kernel {

// Functions below should not be directly used.
namespace __ {

void AssertTrue(bool condition, const char* func_name, int line);
void Panic(const char* func_name, int line);

}  // namespace __
}  // namespace Kernel

#endif
