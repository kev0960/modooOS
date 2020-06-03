#ifndef VGA_OUTPUT_H
#define VGA_OUTPUT_H

#include "../std/algorithm.h"
#include "../std/string_view.h"
#include "../std/type_traits.h"
#include "../std/vector.h"
#include "cpu.h"
#include "keyboard.h"
#include "printf.h"
#include "sync.h"

namespace Kernel {

// Referred from https://os.phil-opp.com/vga-text-mode/
enum VGAColor {
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

class VGAOutput {
 public:
  constexpr static size_t kNumRows = 25;
  constexpr static size_t kNumCols = 80;
  constexpr static uint64_t kVGAMemoryStart =
      (0xffffffff80000000ULL + 0xb8000ULL);

  static VGAOutput& GetVGAOutput() {
    static VGAOutput vga_output;
    return vga_output;
  }

  void PrintLock() { spin_lock_.lock(); }
  void PrintUnlock() { spin_lock_.unlock(); }
  void PrintString(std::string_view s, VGAColor color = White);

  VGAOutput& operator<<(std::string_view s) {
    PrintLock();
    PrintString(s);
    PrintUnlock();
    return (*this);
  }

  template <typename Int,
            std::enable_if_t<std::is_integral<Int>::value, int>* = nullptr>
  VGAOutput& operator<<(Int s) {
    char temp[20];
    ntoa(temp, 20, s, 16);
    PrintLock();
    PrintString(temp);
    PrintUnlock();

    return (*this);
  }

  VGAOutput& operator<<(char c) {
    std::string_view s(&c, 1);
    PrintLock();
    PrintString(s);
    PrintUnlock();

    return (*this);
  }

  void PutCharWithoutLock(char c) {
    std::string_view s(&c, 1);
    PrintString(s);
  }

  void PrintKeyStrokes(const std::vector<KeyStroke>& ks, int start, int end);
  void PutCharAt(size_t row, size_t col, char c, VGAColor color);
  void ClearScreen();

 private:
  size_t current_row_;
  size_t current_col_;

  uint16_t text_buffer_[kNumRows][kNumCols];

  SpinLock m_;

  // Scroll the text buffer up by num_up times.
  void ScrollTextBufferUp(size_t num_up = 1);

  // Print the string at single line.
  void PrintStringLineAtCursor(std::string_view s, VGAColor color) {
    if (current_row_ == kNumRows) {
      ScrollTextBufferUp();
      current_row_--;
    }

    size_t i;
    for (i = current_col_; i < min(current_col_ + s.size(), kNumCols); i++) {
      text_buffer_[current_row_][i] = (s[i - current_col_] | (color << 8));
    }
    current_col_ = i;

    if (current_col_ == kNumCols) {
      current_col_ = 0;
      current_row_++;
    }
  }

  VGAOutput() : current_row_(0), current_col_(0) {
    for (size_t i = 0; i < kNumRows; i++) {
      for (size_t j = 0; j < kNumCols; j++) {
        text_buffer_[i][j] = 0;
      }
    }
  }

  int EstimateNumScrollDown(const std::vector<KeyStroke>& ks, int start,
                            int end);
  void FlushTextBuffer();

  SpinLockNoLockInIntr spin_lock_;
};

}  // namespace Kernel

#endif
