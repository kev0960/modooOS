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

  asm volatile ("int $3");
  asm volatile ("int $4");
  asm volatile ("int $5");
  asm volatile ("int $6");
  asm volatile ("int $7");
  asm volatile ("int $8");
  asm volatile ("int $9");
  asm volatile ("int $10");
  asm volatile ("int $11");
  asm volatile ("int $12");
  while (1) {
  }
}
