#include "stdint.h"

namespace Kernel {
struct IDTEntry {
  uint16_t offset_1;  // offset bits 0..15
  uint16_t selector;  // a code segment selector in GDT or LDT

  // bits 0..2 holds Interrupt Stack Table offset, rest of bits zero.
  uint8_t ist;

  uint8_t type_attr;  // type and attributes
  uint16_t offset_2;  // offset bits 16..31
  uint32_t offset_3;  // offset bits 32..63
  uint32_t zero;      // reserved
} __attribute__((packed));

}  // namespace Kernel
