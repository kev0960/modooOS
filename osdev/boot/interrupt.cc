#include "interrupt.h"
#include "io.h"
#include "types.h"
#include "vga_output.h"
#define CPP_INT
#define UNUSED(expr) \
  do {               \
    (void)(expr);    \
  } while (0)

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

#define ICW4_8086 0x01       /* 8086/88 (MCS-80/85) mode */
#define ICW4_AUTO 0x02       /* Auto (normal) EOI */
#define ICW4_BUF_SLAVE 0x08  /* Buffered mode/slave */
#define ICW4_BUF_MASTER 0x0C /* Buffered mode/master */
#define ICW4_SFNM 0x10       /* Special fully nested (not) */

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

template <int INTERRUPT_INDEX>
__attribute__((interrupt)) void CPUInterruptHandler(
    CPUInterruptHandlerArgs* args) {
  UNUSED(args);
  if (INTERRUPT_INDEX < sizeof(kCPUExceptionErrorMessages)) {
    // vga_output << kCPUExceptionErrorMessages[INTERRUPT_INDEX];
    // Kernel::vga_output << " rbp : " << args->rbp;
    vga_output << " interrupt index : " << INTERRUPT_INDEX;
    vga_output << " \n NO ERROR CODE \n";
    vga_output << " cs : " << args->cs;
    vga_output << " rip : " << args->rip;
    vga_output << " rflags : " << args->rflags;
    vga_output << " rsp : " << args->rsp;
    vga_output << " ss : " << args->ss << "\n";
  } else {
    vga_output << "Error happened! \n";
  }
}

static int cnt = 0;
void DoSomething(uint64_t error_code, uint64_t rip, uint64_t cs,
                 uint64_t rflags, uint64_t rsp, uint64_t ss, int error) {
  vga_output << "error_code : " << error_code << " rip " << rip << "\n";
  vga_output << "cs : " << cs << " rflags " << rflags << " rsp : " << rsp
             << " ss : " << ss << " error : " << error << "\n";
  if (cnt++ >= 10) {
    while (true) {
    }
  }
}

template <int INTERRUPT_INDEX>
__attribute__((interrupt)) void CPUInterruptHandlerWithErrorCode(
    CPUInterruptHandlerArgs* args, uint64_t error) {
  DoSomething(error, args->rip, args->cs, args->rflags, args->rsp, args->ss,
              INTERRUPT_INDEX);
  /*
  vga_output << " rip : " << args->rip << " " << args->rsp << " " << error_code
             << " " << INTERRUPT_INDEX << "\n";
             */
  // while (1) {}
  /*
  if (INTERRUPT_INDEX < sizeof(kCPUExceptionErrorMessages)) {
    //vga_output << kCPUExceptionErrorMessages[INTERRUPT_INDEX];
    // Kernel::vga_output << " rbp : " << args->rbp;
    vga_output << " interrupt index : " << INTERRUPT_INDEX;
    vga_output << " \n NO ERROR CODE ";
    //vga_output << " error code " << error_code;
    //
    vga_output << " cs : " << args->cs;
    vga_output << " rip : " << args->rip;
    vga_output << " rflags : " << args->rflags;
    vga_output << " rsp : " << args->rsp;
    vga_output << " ss : " << args->ss << "\n";
  } else {
    vga_output << "Error happened! \n";
  }
  */
}

template <int INTERRUPT_INDEX>
void InstallIDTEntry(IDTType type_attr, bool has_error_code) {
  auto ih_addr =
      has_error_code
          ? reinterpret_cast<uint64_t>(
                CPUInterruptHandlerWithErrorCode<INTERRUPT_INDEX>)
          : reinterpret_cast<uint64_t>(CPUInterruptHandler<INTERRUPT_INDEX>);

  auto& entry = idt_entries[INTERRUPT_INDEX];

  entry.offset_1 = ih_addr & 0xFFFF;
  entry.selector = 0x8;  // Code segment.
  entry.ist = 0;
  entry.type_attr = type_attr;
  entry.offset_2 = (ih_addr >> 16) & 0xFFFF;
  entry.offset_3 = (ih_addr >> 32) & 0xFFFFFFFF;
  entry.zero = 0;
}

[[maybe_unused]] void InstallIDTEntryAsm(void (*cpu_ih)(), int INTERRUPT_INDEX,
                                         IDTType type_attr) {
  auto ih_addr = reinterpret_cast<uint64_t>(cpu_ih);
  auto& entry = idt_entries[INTERRUPT_INDEX];

  entry.offset_1 = ih_addr & 0xFFFF;
  entry.selector = 0x8;  // Code segment.
  entry.ist = 0;
  entry.type_attr = type_attr;
  entry.offset_2 = (ih_addr >> 16) & 0xFFFF;
  entry.offset_3 = (ih_addr >> 32) & 0xFFFFFFFF;
  entry.zero = 0;
}
}  // namespace

void IDTManager::InitializeIDTForCPUException() {
#ifdef CPP_INT
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
  InstallIDTEntry<31>({INTERRUPT_GATE_32_BIT, 0, 1}, false);
#endif
#ifdef ASM_INT
  InstallIDTEntryAsm(_cpu_ih0, 0, {INTERRUPT_GATE_32_BIT, 0, 1});
  InstallIDTEntryAsm(_cpu_ih1, 1, {INTERRUPT_GATE_32_BIT, 0, 1});
  InstallIDTEntryAsm(_cpu_ih2, 2, {INTERRUPT_GATE_32_BIT, 0, 1});
  InstallIDTEntryAsm(_cpu_ih3, 3, {INTERRUPT_GATE_32_BIT, 0, 1});
  InstallIDTEntryAsm(_cpu_ih4, 4, {INTERRUPT_GATE_32_BIT, 0, 1});
  InstallIDTEntryAsm(_cpu_ih5, 5, {INTERRUPT_GATE_32_BIT, 0, 1});
  InstallIDTEntryAsm(_cpu_ih6, 6, {INTERRUPT_GATE_32_BIT, 0, 1});
  InstallIDTEntryAsm(_cpu_ih7, 7, {INTERRUPT_GATE_32_BIT, 0, 1});
  InstallIDTEntryAsm(_cpu_ih8, 8, {INTERRUPT_GATE_32_BIT, 0, 1});
  InstallIDTEntryAsm(_cpu_ih9, 9, {INTERRUPT_GATE_32_BIT, 0, 1});
  InstallIDTEntryAsm(_cpu_ih10, 10, {INTERRUPT_GATE_32_BIT, 0, 1});
  InstallIDTEntryAsm(_cpu_ih11, 11, {INTERRUPT_GATE_32_BIT, 0, 1});
  InstallIDTEntryAsm(_cpu_ih12, 12, {INTERRUPT_GATE_32_BIT, 0, 1});
  InstallIDTEntryAsm(_cpu_ih13, 13, {INTERRUPT_GATE_32_BIT, 0, 1});
  InstallIDTEntryAsm(_cpu_ih14, 14, {INTERRUPT_GATE_32_BIT, 0, 1});
  InstallIDTEntryAsm(_cpu_ih15, 15, {INTERRUPT_GATE_32_BIT, 0, 1});
  InstallIDTEntryAsm(_cpu_ih16, 16, {INTERRUPT_GATE_32_BIT, 0, 1});
  InstallIDTEntryAsm(_cpu_ih17, 17, {INTERRUPT_GATE_32_BIT, 0, 1});
  InstallIDTEntryAsm(_cpu_ih18, 18, {INTERRUPT_GATE_32_BIT, 0, 1});
  InstallIDTEntryAsm(_cpu_ih19, 19, {INTERRUPT_GATE_32_BIT, 0, 1});
  InstallIDTEntryAsm(_cpu_ih20, 20, {INTERRUPT_GATE_32_BIT, 0, 1});
  InstallIDTEntryAsm(_cpu_ih21, 21, {INTERRUPT_GATE_32_BIT, 0, 1});
  InstallIDTEntryAsm(_cpu_ih22, 22, {INTERRUPT_GATE_32_BIT, 0, 1});
  InstallIDTEntryAsm(_cpu_ih23, 23, {INTERRUPT_GATE_32_BIT, 0, 1});
  InstallIDTEntryAsm(_cpu_ih24, 24, {INTERRUPT_GATE_32_BIT, 0, 1});
  InstallIDTEntryAsm(_cpu_ih25, 25, {INTERRUPT_GATE_32_BIT, 0, 1});
  InstallIDTEntryAsm(_cpu_ih26, 26, {INTERRUPT_GATE_32_BIT, 0, 1});
  InstallIDTEntryAsm(_cpu_ih27, 27, {INTERRUPT_GATE_32_BIT, 0, 1});
  InstallIDTEntryAsm(_cpu_ih28, 28, {INTERRUPT_GATE_32_BIT, 0, 1});
  InstallIDTEntryAsm(_cpu_ih29, 29, {INTERRUPT_GATE_32_BIT, 0, 1});
  InstallIDTEntryAsm(_cpu_ih30, 30, {INTERRUPT_GATE_32_BIT, 0, 1});
  InstallIDTEntryAsm(_cpu_ih31, 31, {INTERRUPT_GATE_32_BIT, 0, 1});
#endif
  // Load IDT to CPU.
  idt_ptr.limit = sizeof(IDTEntry) * 256 - 1;
  idt_ptr.base_addr = reinterpret_cast<uint64_t>(idt_entries);

  /*
  asm volatile(
      "movq %0, %%rax\n\t"
      "lidt (%%rax)"
      :
      : "r"(&idt_ptr)
      : "rax");
  */
  asm volatile("lidt %0" ::"m"(idt_ptr) :);
}

void IDTManager::InitializeIDTForIRQ() {
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
}

}  // namespace Kernel

int cc = 0;
void CPUInterruptHandlerAsm(CPUInterruptHandlerArgsAsm* args) {
  Kernel::vga_output << "error_code : " << args->error_code << " rip "
                     << args->rip << "\n";
  Kernel::vga_output << "cs : " << args->cs << " rflags " << args->rflags
                     << " rsp : " << args->rsp << " ss : " << args->ss
                     << " error : " << args->interrupt_index << "\n";
}
