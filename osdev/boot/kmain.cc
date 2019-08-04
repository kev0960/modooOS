#include "interrupt.h"
#include "vga_output.h"

Kernel::VGAOutput<> Kernel::vga_output{};

extern "C" void KernelMain(void);

void KernelMain() {
    // Initialize Interrupts.
  Kernel::IDTManager idt_manager{};
  idt_manager.InitializeIDTForCPUException();
  Kernel::vga_output << "IDT setup is done! \n";
  /*
  for (int i = 0; i < 20; i++) {
    Kernel::vga_output.PrintString("Hello World!\n", Kernel::VGAColor::Red);
  }*/
  /*
  int *p = reinterpret_cast<int *>(0xFFFFFFFFF);
  *p = 2;
  */
  asm ("int $0x3");
  //Kernel::vga_output  << "hi!";
//  asm volatile("int $3");
//  asm volatile("int $4");

  //asm volatile("int $14");
  /*
  asm volatile ("int $10");
  asm volatile ("int $11");
  asm volatile ("int $12");*/
  while (1) {
  }
}
