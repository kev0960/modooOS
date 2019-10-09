#include "interrupt.h"
#include "vga_output.h"
#include "kernel_test.h"

Kernel::VGAOutput<> Kernel::vga_output{};

extern "C" void KernelMain(void);

void KernelMain() {
  // Initialize Interrupts.
  Kernel::IDTManager idt_manager{};
  idt_manager.InitializeIDTForCPUException();
  idt_manager.InitializeIDTForIRQ();
  idt_manager.LoadIDT();

  Kernel::vga_output << "IDT setup is done! \n";
  Kernel::kernel_test::KernelTestRunner::GetTestRunner().RunTest();

  /*
  asm volatile ("int $10");
  asm volatile ("int $11");
  asm volatile ("int $12");*/
  while (1) {
  }
}
