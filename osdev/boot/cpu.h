#ifndef CPU_H
#define CPU_H

#include "types.h"

namespace Kernel {

inline uint64_t GetRFlags() {
  uint64_t rflags;
  asm volatile(
      "pushfq \n"
      "popq %0"
      : "=rm"(rflags)::"memory");
  return rflags;
}

inline void SetRFlags(uint64_t rflags) {
  asm volatile(
      "pushq %0\n"
      "popfq" ::"g"(rflags)
      : "memory", "cc");
}

inline void EnableInterrupt() { asm volatile("sti"); }

inline void DisableInterrupt() { asm volatile("cli" ::: "memory"); }
}  // namespace Kernel

#endif
