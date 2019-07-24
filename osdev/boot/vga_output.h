#ifndef VGA_OUTPUT_H
#define VGA_OUTPUT_H

#include "string_view.h"

namespace Kernel {
class VGAOutput {
 public:

  // Referred from https://os.phil-opp.com/vga-text-mode/
  enum Color {
    Black = 0,
    Blue = 1,
    Green = 2,
    Cyan = 3,
    Red = 4,
    Magenta = 5,
    Brown = 6,
    LightGray = 7,
    DarkGray = 8,
    LightBlue = 9,
    LightGreen = 10,
    LightCyan = 11,
    LightRed = 12,
    Pink = 13,
    Yellow = 14,
    White = 15,
  };

  explicit VGAOutput(int num_rows = 80, int num_cols = 25)
      : num_rows_(num_rows), num_cols_(num_cols) {}
  void PrintString(string_view s, Color color = White);

 private:
  const int num_rows_;
  const int num_cols_;

  int cursor;
};
}  // namespace Kernel

#endif
