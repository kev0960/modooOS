#include "vga_output.h"
#include "interrupt.h"

Kernel::VGAOutput<> Kernel::vga_output{};

extern "C" void KernelMain(void);

void KernelMain() {
  for (int i = 0; i < 20; i++) {
    Kernel::vga_output.PrintString("Hello World!", Kernel::VGAColor::Red);
  }
  for (int i = 0; i < 10; i++) {
    Kernel::vga_output.PrintString("Hello World!", Kernel::VGAColor::Green);
  }
  for (int i = 0; i < 5; i++) {
    Kernel::vga_output.PrintString("Hello World!", Kernel::VGAColor::Blue);
  }

  // Initialize Interrupts.
  Kernel::IDTManager idt_manager{};
  idt_manager.InitializeIDT();

  asm volatile ("int $13");
  asm volatile ("int $14");
  asm volatile ("int $14");

  while (1) {
  }
}
