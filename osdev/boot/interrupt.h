#ifndef INTERRUPT_H
#define INTERRUPT_H

#include "stdint.h"

namespace Kernel {

struct IDTType {
  uint8_t gate_type : 5;
  uint8_t dpl : 2;
  uint8_t p : 1;
};

// Figure 6-6 Intel-3
struct IDTEntry {
  uint16_t offset_1;  // offset bits 0..15
  uint16_t selector;  // a code segment selector in GDT or LDT

  // bits 0..2 holds Interrupt Stack Table offset, rest of bits zero.
  uint8_t ist;

  IDTType type_attr;  // type and attributes
  uint16_t offset_2;  // offset bits 16..31
  uint32_t offset_3;  // offset bits 32..63
  uint32_t zero;      // reserved
} __attribute__((packed));

struct IDTR {
  uint16_t limit;
  uint64_t base_addr;
} __attribute__((packed));

class IDTManager {
 public:
  IDTManager() = default;

  void InitializeIDTForCPUException();
  void InitializeIDTForIRQ();
  void LoadIDT();
};

struct CPUInterruptHandlerArgs {
  uint64_t rip;
  uint64_t cs;
  uint64_t rflags;
  uint64_t rsp;
  uint64_t ss;
} __attribute__((packed)) ;

}  // namespace Kernel

#endif
