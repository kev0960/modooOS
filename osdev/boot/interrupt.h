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
};

// Interrupt handlers; Defined in interrupt_asm.S
extern "C" {
void _cpu_ih0();
void _cpu_ih1();
void _cpu_ih2();
void _cpu_ih3();
void _cpu_ih4();
void _cpu_ih5();
void _cpu_ih6();
void _cpu_ih7();
void _cpu_ih8();
void _cpu_ih9();
void _cpu_ih10();
void _cpu_ih11();
void _cpu_ih12();
void _cpu_ih13();
void _cpu_ih14();
void _cpu_ih15();
void _cpu_ih16();
void _cpu_ih17();
void _cpu_ih18();
void _cpu_ih19();
void _cpu_ih20();
void _cpu_ih21();
void _cpu_ih22();
void _cpu_ih23();
void _cpu_ih24();
void _cpu_ih25();
void _cpu_ih26();
void _cpu_ih27();
void _cpu_ih28();
void _cpu_ih29();
void _cpu_ih30();
void _cpu_ih31();
}

}  // namespace Kernel

struct CPUInterruptHandlerArgsWithErrorCode {
  uint64_t error_code;
  uint64_t rip;
  uint64_t cs;
  uint64_t rflags;
  uint64_t rsp;
  uint64_t ss;
} __attribute__((packed)) ;

struct CPUInterruptHandlerArgs {
  uint64_t rip;
  uint64_t cs;
  uint64_t rflags;
  uint64_t rsp;
  uint64_t ss;
} __attribute__((packed)) ;

struct CPUInterruptHandlerArgsAsm {
  uint64_t rbp;
  uint64_t interrupt_index;
  uint64_t error_code;
  uint64_t rip;
  uint64_t cs;
  uint64_t rflags;
  uint64_t rsp;
  uint64_t ss;
} __attribute__((packed)) ;


// CPU Exception handler (should be outside of the namespace to be called from
// assembly)
extern "C" {
void CPUInterruptHandlerAsm(CPUInterruptHandlerArgsAsm* args);
}
#endif
