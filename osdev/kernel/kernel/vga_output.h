#ifndef VGA_OUTPUT_H
#define VGA_OUTPUT_H

#include "../std/algorithm.h"
#include "../std/string_view.h"
#include "../std/type_traits.h"
#include "cpu.h"
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

template <size_t NUM_ROWS = 25, size_t NUM_COLS = 80>
class VGAOutput {
 public:
  constexpr static uint64_t kVGAMemoryStart =
      0xffffffff80000000ULL + 0xb8000ULL;

  explicit VGAOutput(int offset = 0)
      : num_rows_(NUM_ROWS),
        num_cols_(NUM_COLS),
        current_row_(0),
        current_col_(0),
        offset_(offset) {
    for (size_t i = 0; i < num_rows_; i++) {
      for (size_t j = 0; j < num_cols_; j++) {
        text_buffer_[i][j] = 0;
      }
    }
  }

  void PrintString(std::string_view s, VGAColor color = White) {
    while (!s.empty()) {
      auto len = min(num_cols_ - current_col_, s.size());
      size_t endline_or_col_chars = s.find_first_of('\n', 0, len);
      bool endline_found = (endline_or_col_chars != npos);

      if (endline_or_col_chars == npos) {
        endline_or_col_chars = len;
      }
      auto first_num_col_chars = s.substr(0, endline_or_col_chars);

      PrintStringLineAtCursor(first_num_col_chars, color);

      if (endline_found && current_col_ != 0 && current_row_ < NUM_ROWS) {
        // Fill remaining part as 0.
        for (size_t i = current_col_; i < num_cols_; i++) {
          text_buffer_[current_row_][i] = 0;
        }

        current_row_++;
        current_col_ = 0;
      }

      if (endline_found) {
        s.remove_prefix(min(endline_or_col_chars + 1, len));
      } else {
        s.remove_prefix(min(endline_or_col_chars, len));
      }
    }

    auto vga = reinterpret_cast<uint16_t*>(kVGAMemoryStart + offset_);
    for (size_t i = 0; i < current_row_; i++) {
      for (size_t j = 0; j < num_cols_; j++) {
        vga[i * num_cols_ + j] = text_buffer_[i][j];
      }
    }
    if (current_col_ != 0) {
      for (size_t i = 0; i < num_cols_; i++) {
        vga[num_cols_ * current_row_ + i] = text_buffer_[current_row_][i];
      }
    }
  }

  VGAOutput<NUM_ROWS, NUM_COLS>& operator<<(std::string_view s) {
    PrintString(s);
    return (*this);
  }

  template <typename Int,
            std::enable_if_t<std::is_integral<Int>::value, int>* = nullptr>
  VGAOutput<NUM_ROWS, NUM_COLS>& operator<<(Int s) {
    char temp[20];
    ntoa(temp, 20, s, 16);
    PrintString(temp);

    return (*this);
  }

  VGAOutput<NUM_ROWS, NUM_COLS>& operator<<(char c) {
    std::string_view s(&c, 1);
    PrintString(s);
    return (*this);
  }

  const size_t num_rows_;
  const size_t num_cols_;

  size_t current_row_;
  size_t current_col_;

  size_t offset_;

  uint16_t text_buffer_[NUM_ROWS][NUM_COLS];

  SpinLock m_;

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

  // Print the string at single line.
  void PrintStringLineAtCursor(std::string_view s, VGAColor color) {
    if (current_row_ == num_rows_) {
      ScrollTextBufferUp();
      current_row_--;
    }

    size_t i;
    for (i = current_col_; i < min(current_col_ + s.size(), num_cols_); i++) {
      text_buffer_[current_row_][i] = (s[i - current_col_] | (color << 8));
    }
    current_col_ = i;

    if (current_col_ == num_cols_) {
      current_col_ = 0;
      current_row_++;
    }
  }
};

extern VGAOutput<> vga_output;

}  // namespace Kernel

#endif
