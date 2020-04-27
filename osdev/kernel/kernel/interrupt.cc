#include "interrupt.h"

#include "../std/types.h"
#include "./fs/ata.h"
#include "apic.h"
#include "cpp_macro.h"
#include "io.h"
#include "keyboard.h"
#include "paging.h"
#include "scheduler.h"
#include "timer.h"
#include "vga_output.h"

#define INTERRUPT_GATE_32_BIT (0b01110)
#define TRAP_GATE_32_BIT (0b01111)

#define PIC_MASTER 0x20 /* IO base address for master PIC */
#define PIC_SLAVE 0xA0  /* IO base address for slave PIC */
#define PIC_MASTER_COMMAND PIC_MASTER
#define PIC_MASTER_DATA (PIC_MASTER + 1)
#define PIC_SLAVE_COMMAND PIC_SLAVE
#define PIC_SLAVE_DATA (PIC_SLAVE + 1)

#define ICW1_ICW4 0x01      /* ICW4 (not) needed */
#define ICW1_SINGLE 0x02    /* Single (cascade) mode */
#define ICW1_INTERVAL4 0x04 /* Call address interval 4 (8) */
#define ICW1_LEVEL 0x08     /* Level triggered (edge) mode */
#define ICW1_INIT 0x10      /* Initialization - required! */

#define ICW4_8086 0x01 /* 8086/88 (MCS-80/85) mode */

Kernel::IDTEntry idt_entries[256];
Kernel::IDTR idt_ptr;

namespace Kernel {
namespace {

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

void PrintCPUInterruptFrame(CPUInterruptHandlerArgs* args, size_t int_num) {
  vga_output << "Interrupt Frame --------------------\n";
  vga_output << " Interrupt index : " << int_num;
  if (int_num < sizeof(kCPUExceptionErrorMessages) /
                    sizeof(kCPUExceptionErrorMessages[0])) {
    vga_output << " [" << kCPUExceptionErrorMessages[int_num] << "]";
  }
  vga_output << "\n";
  vga_output << "at : " << KernelThread::CurrentThread()->Id();
  vga_output << " cs : " << args->cs << "\n";
  vga_output << " rip : " << args->rip << "\n";
  vga_output << " rflags : " << args->rflags << "\n";
  vga_output << " rsp : " << args->rsp << "\n";
  vga_output << " ss : " << args->ss << "\n";

  while (1)
    ;
}

template <int INT_NUM>
__attribute__((interrupt)) void CPUInterruptHandler(
    CPUInterruptHandlerArgs* args) {
  PrintCPUInterruptFrame(args, INT_NUM);
}

template <int INT_NUM>
__attribute__((interrupt)) void CPUInterruptHandlerWithErrorCode(
    CPUInterruptHandlerArgs* args, uint64_t error_code) {
  vga_output << "Error Code : " << error_code << "\n";
  PrintCPUInterruptFrame(args, INT_NUM);
}

template <int INT_NUM>
void InstallIDTEntry(IDTType type_attr, bool has_error_code) {
  auto ih_addr = has_error_code
                     ? reinterpret_cast<uint64_t>(
                           CPUInterruptHandlerWithErrorCode<INT_NUM>)
                     : reinterpret_cast<uint64_t>(CPUInterruptHandler<INT_NUM>);

  if constexpr (INT_NUM == 0xE) {
    ih_addr = reinterpret_cast<uint64_t>(PageFaultInterruptHandler);
  }

  auto& entry = idt_entries[INT_NUM];

  entry.offset_1 = ih_addr & 0xFFFF;
  entry.selector = 0x8;  // Code segment.
  entry.ist = 0;
  entry.type_attr = type_attr;
  entry.offset_2 = (ih_addr >> 16) & 0xFFFF;
  entry.offset_3 = (ih_addr >> 32) & 0xFFFFFFFF;
  entry.zero = 0;
}

void InstallIDTEntry(void (*handler)(CPUInterruptHandlerArgs*),
                     IDTType type_attr, size_t int_num) {
  auto ih_addr = reinterpret_cast<uint64_t>(handler);
  auto& entry = idt_entries[int_num];

  entry.offset_1 = ih_addr & 0xFFFF;
  entry.selector = 0x8;  // Code segment.
  entry.ist = 0;
  entry.type_attr = type_attr;
  entry.offset_2 = (ih_addr >> 16) & 0xFFFF;
  entry.offset_3 = (ih_addr >> 32) & 0xFFFFFFFF;
  entry.zero = 0;
}

void InstallIDTEntry(void (*handler)(), IDTType type_attr, size_t int_num) {
  auto ih_addr = reinterpret_cast<uint64_t>(handler);
  auto& entry = idt_entries[int_num];

  entry.offset_1 = ih_addr & 0xFFFF;
  entry.selector = 0x8;  // Code segment.
  entry.ist = 0;
  entry.type_attr = type_attr;
  entry.offset_2 = (ih_addr >> 16) & 0xFFFF;
  entry.offset_3 = (ih_addr >> 32) & 0xFFFFFFFF;
  entry.zero = 0;
}

[[maybe_unused]] void IRQSetMask(uint8_t irq_line) {
  uint16_t port;
  uint8_t value;

  if (irq_line < 8) {
    port = PIC_MASTER_DATA;
  } else {
    port = PIC_SLAVE_DATA;
    irq_line -= 8;
  }
  value = inb(port) | (1 << irq_line);
  outb(port, value);
}

[[maybe_unused]] void IRQClearMask(uint8_t irq_line) {
  uint16_t port;
  uint8_t value;

  if (irq_line < 8) {
    port = PIC_MASTER_DATA;
  } else {
    port = PIC_SLAVE_DATA;
    irq_line -= 8;
  }
  value = inb(port) & ~(1 << irq_line);
  outb(port, value);
}

inline void EndOfIRQ() { outb(0x20, 0x20); }
inline void EndOfIRQForSlave() { outb(0xA0, 0x20); }

}  // namespace

__attribute__((interrupt)) void KeyboardHandler(CPUInterruptHandlerArgs* args) {
  UNUSED(args);

  uint8_t scan_code = inb(0x60);
  ps2_keyboard.MainKeyboardHandler(scan_code);
  EndOfIRQ();

  if (APICManager::GetAPICManager().IsMulticoreEnabled()) {
    APICManager::GetAPICManager().SetEndOfInterrupt();
  }
}

__attribute__((interrupt)) void ATAHandler(CPUInterruptHandlerArgs* args) {
  UNUSED(args);

  EndOfIRQForSlave();
  EndOfIRQ();

  kATADiskCommandSema.Up();
}

/*
__attribute__((interrupt)) void ATAHandler2(CPUInterruptHandlerArgs* args) {
  UNUSED(args);

  EndOfIRQForSlave();
  EndOfIRQ();

  kATADiskCommandSema.Up(true);
}*/

void IDTManager::InitializeIDTForCPUException() {
  InstallIDTEntry<0>({INTERRUPT_GATE_32_BIT, 0, 1}, false);
  InstallIDTEntry<1>({INTERRUPT_GATE_32_BIT, 0, 1}, false);
  InstallIDTEntry<2>({INTERRUPT_GATE_32_BIT, 0, 1}, false);
  InstallIDTEntry<3>({INTERRUPT_GATE_32_BIT, 0, 1}, false);
  InstallIDTEntry<4>({INTERRUPT_GATE_32_BIT, 0, 1}, false);
  InstallIDTEntry<5>({INTERRUPT_GATE_32_BIT, 0, 1}, false);
  InstallIDTEntry<6>({INTERRUPT_GATE_32_BIT, 0, 1}, false);
  InstallIDTEntry<7>({INTERRUPT_GATE_32_BIT, 0, 1}, false);
  InstallIDTEntry<8>({INTERRUPT_GATE_32_BIT, 0, 1}, true);
  InstallIDTEntry<9>({INTERRUPT_GATE_32_BIT, 0, 1}, false);
  InstallIDTEntry<10>({INTERRUPT_GATE_32_BIT, 0, 1}, true);
  InstallIDTEntry<11>({INTERRUPT_GATE_32_BIT, 0, 1}, true);
  InstallIDTEntry<12>({INTERRUPT_GATE_32_BIT, 0, 1}, true);
  InstallIDTEntry<13>({INTERRUPT_GATE_32_BIT, 0, 1}, true);
  InstallIDTEntry<14>({INTERRUPT_GATE_32_BIT, 0, 1}, true);
  InstallIDTEntry<15>({INTERRUPT_GATE_32_BIT, 0, 1}, false);
  InstallIDTEntry<16>({INTERRUPT_GATE_32_BIT, 0, 1}, false);
  InstallIDTEntry<17>({INTERRUPT_GATE_32_BIT, 0, 1}, true);
  InstallIDTEntry<18>({INTERRUPT_GATE_32_BIT, 0, 1}, false);
  InstallIDTEntry<19>({INTERRUPT_GATE_32_BIT, 0, 1}, false);
  InstallIDTEntry<20>({INTERRUPT_GATE_32_BIT, 0, 1}, false);
  InstallIDTEntry<21>({INTERRUPT_GATE_32_BIT, 0, 1}, false);
  InstallIDTEntry<22>({INTERRUPT_GATE_32_BIT, 0, 1}, false);
  InstallIDTEntry<23>({INTERRUPT_GATE_32_BIT, 0, 1}, false);
  InstallIDTEntry<24>({INTERRUPT_GATE_32_BIT, 0, 1}, false);
  InstallIDTEntry<25>({INTERRUPT_GATE_32_BIT, 0, 1}, false);
  InstallIDTEntry<26>({INTERRUPT_GATE_32_BIT, 0, 1}, false);
  InstallIDTEntry<27>({INTERRUPT_GATE_32_BIT, 0, 1}, false);
  InstallIDTEntry<28>({INTERRUPT_GATE_32_BIT, 0, 1}, false);
  InstallIDTEntry<29>({INTERRUPT_GATE_32_BIT, 0, 1}, false);
  InstallIDTEntry<30>({INTERRUPT_GATE_32_BIT, 0, 1}, false);
  InstallIDTEntry<31>({INTERRUPT_GATE_32_BIT, 0, 1}, false);  // 0x19
}

void IDTManager::LoadIDT() {
  // Load IDT to CPU.
  idt_ptr.limit = sizeof(IDTEntry) * 256 - 1;
  idt_ptr.base_addr = reinterpret_cast<uint64_t>(idt_entries);
  asm volatile("lidt %0" ::"m"(idt_ptr) :);
  asm volatile("sti");
}
void IDTManager::InitializeIDTForIRQ() {
  TimerManager::GetTimerManager().InstallPICTimer();

  InstallIDTEntry(TimerInterruptHandler, {INTERRUPT_GATE_32_BIT, 0, 1}, 0x20);
  InstallIDTEntry(KeyboardHandler, {INTERRUPT_GATE_32_BIT, 0, 1}, 0x21);
  InstallIDTEntry(ATAHandler, {INTERRUPT_GATE_32_BIT, 0, 1}, 0x2E);
  InstallIDTEntry(ATAHandler, {INTERRUPT_GATE_32_BIT, 0, 1}, 0x2F);

  // PIC Remap.

  // Save previous masks.
  uint8_t mask_master, mask_slave;

  mask_master = inb(PIC_MASTER_DATA);
  mask_slave = inb(PIC_SLAVE_DATA);

  // Send PIC Initialization command.
  outb(PIC_MASTER_COMMAND, ICW1_INIT | ICW1_ICW4);
  outb(PIC_SLAVE_COMMAND, ICW1_INIT | ICW1_ICW4);

  // Set the offset of interrupt vectors (so that it does not overlap with CPU
  // exceptions).
  outb(PIC_MASTER_DATA, 0x20);
  outb(PIC_SLAVE_DATA, 0x28);

  // Tell master that it has slave PIC on IRQ2 (0000 0100 = 4).
  outb(PIC_MASTER_DATA, 4);
  // Tell slave to cascade its identity.
  outb(PIC_SLAVE_DATA, 2);

  outb(PIC_MASTER_DATA, ICW4_8086);
  outb(PIC_SLAVE_DATA, ICW4_8086);

  // Restore saved masks.
  outb(PIC_MASTER_DATA, mask_master);
  outb(PIC_SLAVE_DATA, mask_slave);

  IRQClearMask(0);
  IRQClearMask(1);
  IRQClearMask(14);
  IRQClearMask(15);

  // outb(PIC_MASTER_DATA, 0xFD);
  // outb(PIC_SLAVE_DATA, 0xFF);
}

void IDTManager::InitializeCustomInterrupt() {
  // From 0x30 ~, we can use our own IRQs.
  InstallIDTEntry(CustomContextSwitchInterruptHandler,
                  {INTERRUPT_GATE_32_BIT, 0, 1}, 0x30);
}

void IDTManager::DisablePIC() {
  outb(PIC_SLAVE_DATA, 0xFF);
  outb(PIC_MASTER_DATA, 0xFF);
}

}  // namespace Kernel
