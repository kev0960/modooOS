#include "vga_output.h"

extern "C" void KernelMain(void);

void KernelMain() {
  Kernel::VGAOutput vga_io{};
  vga_io.PrintString("Hello World!", Kernel::VGAOutput::Red);

  while (1) {
  }
}
