#include "vga_output.h"
#include "string_util.h"

namespace Kernel {
void VGAOutput::PrintString(string_view s, Color color) {
  auto len = s.size();
  short* vga = reinterpret_cast<short*>(0xb8000);
  for (unsigned int i = 0; i < len; i++) {
    short vga_char = s[i];
    vga[i] = (vga_char | (color << 8));
  }
}
}  // namespace Kernel
