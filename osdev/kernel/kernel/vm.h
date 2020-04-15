#ifndef VM_H
#define VM_H

#include "../std/types.h"

namespace Kernel {

static constexpr uint64_t kKernelOffset = 0xffffffff80000000ULL;

template <typename T, typename U>
constexpr U PhysToKernelVirtual(T phys_addr) {
  return reinterpret_cast<U>(reinterpret_cast<uint64_t>(phys_addr) +
                             kKernelOffset);
}

template <typename T, typename U>
constexpr U KernelVirtualToPhys(T virtual_addr) {
  return reinterpret_cast<U>(reinterpret_cast<uint64_t>(virtual_addr) -
                             kKernelOffset);
}

}  // namespace Kernel
#endif
