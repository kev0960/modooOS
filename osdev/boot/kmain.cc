#include "vga_output.h"
#include "interrupt.h"

Kernel::VGAOutput<> Kernel::vga_output{};

extern "C" void KernelMain(void);

void KernelMain() {
  /*
  for (int i = 0; i < 20; i++) {
    Kernel::vga_output.PrintString("Hello World!", Kernel::VGAColor::Red);
  }
  for (int i = 0; i < 10; i++) {
    Kernel::vga_output.PrintString("Hello World!", Kernel::VGAColor::Green);
  }
  for (int i = 0; i < 5; i++) {
    Kernel::vga_output.PrintString("Hello World!", Kernel::VGAColor::Blue);
  }
  */

  // Initialize Interrupts.
  Kernel::IDTManager idt_manager{};
  idt_manager.InitializeIDT();

  asm volatile ("int $10");
  asm volatile ("int $11");
  asm volatile ("int $12");
  asm volatile ("int $13");
  asm volatile ("int $14");
  asm volatile ("int $15");
  asm volatile ("int $16");
  asm volatile ("int $17");
  asm volatile ("int $18");
  asm volatile ("int $19");
  while (1) {
  }
}
