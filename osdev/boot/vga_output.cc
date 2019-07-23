#include "vga_output.h"
#include "string_util.h"

namespace Kernel {
  void VGAOutput::PrintString(const char* s, Color color) {
    auto len = strlen(s);

    short* vga = reinterpret_cast<short *>(0xb8000);
    for (unsigned int i = 0; i < len; i ++) {
      short vga_char = s[i];
      vga[i] = (vga_char | (color << 8));
    }
  }
}
