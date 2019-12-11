#ifndef IO_H
#define IO_H

#include "types.h"

namespace Kernel {
static inline void outb(uint16_t port, uint8_t val) {
  // Copy value to EAX and port to EDX.
  asm volatile("outb %0, %1" ::"a"(val), "Nd"(port) :);
}

static inline uint8_t inb(uint16_t port) {
  uint8_t ret;
  asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port) :);
  return ret;
}

static inline void outw(uint16_t port, uint16_t val) {
  // Copy value to EAX and port to EDX.
  asm volatile("outw %0, %1" ::"a"(val), "Nd"(port) :);
}

static inline uint16_t inw(uint16_t port) {
  uint16_t ret;
  asm volatile("inw %1, %0" : "=a"(ret) : "Nd"(port) :);
  return ret;
}

}  // namespace Kernel

#endif
