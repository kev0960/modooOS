#include "interrupt.h"
#include "types.h"
#include "vga_output.h"

#define INTERRUPT_GATE_32_BIT (0b01110)
#define TRAP_GATE_32_BIT (0b01111)

Kernel::IDTEntry idt_entries[256];
Kernel::IDTR idt_ptr;

namespace Kernel {
namespace {
void InstallIDTEntry(void (*cpu_ih)(), size_t interrupt_num,
                     IDTType type_attr) {
  uint64_t ih_addr = reinterpret_cast<uint64_t>(cpu_ih);

  auto& entry = idt_entries[interrupt_num];

  entry.offset_1 = ih_addr & 0xFFFF;
  entry.selector = 0x8;  // Code segment.
  entry.ist = 0;
  entry.type_attr = type_attr;
  entry.offset_2 = (ih_addr >> 16) & 0xFFFF;
  entry.offset_3 = (ih_addr >> 32) & 0xFFFFFFFF;
  entry.zero = 0;
}

}  // namespace

void IDTManager::InitializeIDT() {
  InstallIDTEntry(_cpu_ih0, 0, {INTERRUPT_GATE_32_BIT, 0, 1});
  InstallIDTEntry(_cpu_ih1, 1, {INTERRUPT_GATE_32_BIT, 0, 1});
  InstallIDTEntry(_cpu_ih2, 2, {INTERRUPT_GATE_32_BIT, 0, 1});
  InstallIDTEntry(_cpu_ih3, 3, {INTERRUPT_GATE_32_BIT, 0, 1});
  InstallIDTEntry(_cpu_ih4, 4, {INTERRUPT_GATE_32_BIT, 0, 1});
  InstallIDTEntry(_cpu_ih5, 5, {INTERRUPT_GATE_32_BIT, 0, 1});
  InstallIDTEntry(_cpu_ih6, 6, {INTERRUPT_GATE_32_BIT, 0, 1});
  InstallIDTEntry(_cpu_ih7, 7, {INTERRUPT_GATE_32_BIT, 0, 1});
  InstallIDTEntry(_cpu_ih8, 8, {INTERRUPT_GATE_32_BIT, 0, 1});
  InstallIDTEntry(_cpu_ih9, 9, {INTERRUPT_GATE_32_BIT, 0, 1});
  InstallIDTEntry(_cpu_ih10, 10, {INTERRUPT_GATE_32_BIT, 0, 1});
  InstallIDTEntry(_cpu_ih11, 11, {INTERRUPT_GATE_32_BIT, 0, 1});
  InstallIDTEntry(_cpu_ih12, 12, {INTERRUPT_GATE_32_BIT, 0, 1});
  InstallIDTEntry(_cpu_ih13, 13, {INTERRUPT_GATE_32_BIT, 0, 1});
  InstallIDTEntry(_cpu_ih14, 14, {INTERRUPT_GATE_32_BIT, 0, 1});
  InstallIDTEntry(_cpu_ih15, 15, {INTERRUPT_GATE_32_BIT, 0, 1});
  InstallIDTEntry(_cpu_ih16, 16, {INTERRUPT_GATE_32_BIT, 0, 1});
  InstallIDTEntry(_cpu_ih17, 17, {INTERRUPT_GATE_32_BIT, 0, 1});
  InstallIDTEntry(_cpu_ih18, 18, {INTERRUPT_GATE_32_BIT, 0, 1});
  InstallIDTEntry(_cpu_ih19, 19, {INTERRUPT_GATE_32_BIT, 0, 1});
  InstallIDTEntry(_cpu_ih20, 20, {INTERRUPT_GATE_32_BIT, 0, 1});
  InstallIDTEntry(_cpu_ih21, 21, {INTERRUPT_GATE_32_BIT, 0, 1});
  InstallIDTEntry(_cpu_ih22, 22, {INTERRUPT_GATE_32_BIT, 0, 1});
  InstallIDTEntry(_cpu_ih23, 23, {INTERRUPT_GATE_32_BIT, 0, 1});
  InstallIDTEntry(_cpu_ih24, 24, {INTERRUPT_GATE_32_BIT, 0, 1});
  InstallIDTEntry(_cpu_ih25, 25, {INTERRUPT_GATE_32_BIT, 0, 1});
  InstallIDTEntry(_cpu_ih26, 26, {INTERRUPT_GATE_32_BIT, 0, 1});
  InstallIDTEntry(_cpu_ih27, 27, {INTERRUPT_GATE_32_BIT, 0, 1});
  InstallIDTEntry(_cpu_ih28, 28, {INTERRUPT_GATE_32_BIT, 0, 1});
  InstallIDTEntry(_cpu_ih29, 29, {INTERRUPT_GATE_32_BIT, 0, 1});
  InstallIDTEntry(_cpu_ih30, 30, {INTERRUPT_GATE_32_BIT, 0, 1});
  InstallIDTEntry(_cpu_ih31, 31, {INTERRUPT_GATE_32_BIT, 0, 1});

  // Load IDT to CPU.
  idt_ptr.limit = sizeof(IDTEntry) * 256 - 1;
  idt_ptr.base_addr = reinterpret_cast<uint64_t>(idt_entries);

  asm volatile(
      "movq %0, %%rax\n\t"
      "lidt (%%rax)"
      :
      : "r"(&idt_ptr)
      : "rax");
}

static const char* kCPUExceptionErrorMessages[] = {
    "#DE",           "#DB",
    "NMI Interrupt", "#BP",
    "#OF",           "#BR",
    "#UD",           "#MM",
    "#DF",           "INT 9; DO NOT USE",
    "#TS",           "#NP",
    "#SS",           "#GP",
    "#PF",           "INT 15; DO NOT USE",
    "#MF",           "#AC",
    "#MC",           "#XM",
    "#VE",
};

}  // namespace Kernel

void CPUInterruptHandler(CPUInterruptHandlerArgs* args) {
  if (args->interrupt_index < sizeof(Kernel::kCPUExceptionErrorMessages)) {
    Kernel::vga_output
        << Kernel::kCPUExceptionErrorMessages[args->interrupt_index];
    Kernel::vga_output << " rip : " << args->rip;
    Kernel::vga_output << " rsp : " << args->rsp;
    Kernel::vga_output << " rbp : " << args->rbp;
    Kernel::vga_output << "error code : " << args->error_code;
  } else {
    Kernel::vga_output << "Error happened!";
  }
}
