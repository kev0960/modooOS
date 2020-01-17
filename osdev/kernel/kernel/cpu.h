#ifndef CPU_H
#define CPU_H

#include "../std/types.h"

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

inline bool IsInterruptEnabled() {
  auto rflags = GetRFlags();
  return rflags & 0x200;
}

// Get and Set model specific registers.
// Courtesy of osdev. https://wiki.osdev.org/Model_Specific_Registers
inline void GetMSR(uint32_t msr, uint32_t *lo, uint32_t *hi) {
  asm volatile("rdmsr" : "=a"(*lo), "=d"(*hi) : "c"(msr));
}

inline void SetMSR(uint32_t msr, uint32_t lo, uint32_t hi) {
  asm volatile("wrmsr" : : "a"(lo), "d"(hi), "c"(msr));
}

inline void EnableInterrupt() { asm volatile("sti"); }
inline void DisableInterrupt() { asm volatile("cli" ::: "memory"); }

inline uint64_t ReadCR0() {
  uint64_t cr0;
  asm volatile(
      "mov %%cr0, %%rax\n"
      "mov %%rax, %0"
      : "=m"(cr0)::"%rax");
  return cr0;
}

}  // namespace Kernel

#endif