#include "vga_output.h"

extern "C" void KernelMain(void);

void KernelMain() {
  Kernel::VGAOutput<25, 80> vga_io{};
  for (int i = 0; i < 20; i++) {
    vga_io.PrintString("Hello World!", Kernel::VGAColor::Red);
  }
  for (int i = 0; i < 10; i++) {
    vga_io.PrintString("Hello World!", Kernel::VGAColor::Green);
  }
  for (int i = 0; i < 5; i++) {
    vga_io.PrintString("Hello World!", Kernel::VGAColor::Blue);
  }

  while (1) {
  }
}
