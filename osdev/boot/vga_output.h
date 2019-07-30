#ifndef VGA_OUTPUT_H
#define VGA_OUTPUT_H

#include "algorithm.h"
#include "string_view.h"
#include "type_traits.h"

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

template <size_t NUM_ROWS = 25, size_t NUM_COLS = 80>
class VGAOutput {
 public:
  explicit VGAOutput()
      : num_rows_(NUM_ROWS), num_cols_(NUM_COLS), current_row_(0) {
    for (size_t i = 0; i < num_rows_; i++) {
      for (size_t j = 0; j < num_cols_; j++) {
        text_buffer_[i][j] = 0;
      }
    }
  }

  void PrintString(string_view s, VGAColor color = White) {
    while (!s.empty()) {
      auto len = min(num_cols_, s.size());
      auto first_num_col_chars = s.substr(0, len);

      PrintStringLineAtCursor(first_num_col_chars, color);
      s.remove_prefix(len);
    }

    short* vga = reinterpret_cast<short*>(0xb8000);
    for (size_t i = 0; i < current_row_; i++) {
      for (size_t j = 0; j < num_cols_; j++) {
        if (!text_buffer_[i][j]) {
          break;
        }
        vga[i * num_cols_ + j] = text_buffer_[i][j];
      }
    }
  }

  VGAOutput<NUM_ROWS, NUM_COLS>& operator<<(string_view s) {
    PrintString(s);
    return (*this);
  }

  template <typename Int, enable_if_t<is_integral<Int>::value, int>* = nullptr>
  VGAOutput<NUM_ROWS, NUM_COLS>& operator<<(Int s) {
    char temp[20];
    ntoa(temp, 20, s, 16);
    PrintString(temp);

    return (*this);
  }

 private:
  const size_t num_rows_;
  const size_t num_cols_;

  size_t current_row_;

  short text_buffer_[NUM_ROWS][NUM_COLS];

  // Scroll the text buffer up.
  void ScrollTextBufferUp() {
    for (size_t i = 1; i < num_rows_; i++) {
      for (size_t j = 0; j < num_cols_; j++) {
        text_buffer_[i - 1][j] = text_buffer_[i][j];
      }
    }

    // Clear the last row.
    for (size_t i = 0; i < num_cols_; i++) {
      text_buffer_[num_rows_ - 1][i] = 0;
    }
  }

  // Print the string at single line. Any string longer than num_cols will be
  // truncated.
  void PrintStringLineAtCursor(string_view s, VGAColor color) {
    if (current_row_ == num_rows_) {
      ScrollTextBufferUp();
      current_row_--;
    }

    for (size_t i = 0; i < min(s.size(), num_cols_); i++) {
      text_buffer_[current_row_][i] = (s[i] | (color << 8));
    }
    current_row_++;
  }
};

extern VGAOutput<> vga_output;
}  // namespace Kernel

#endif
