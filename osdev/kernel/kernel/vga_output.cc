#include "vga_output.h"

#include "qemu_log.h"

namespace Kernel {

void VGAOutput::PrintString(std::string_view s, VGAColor color) {
  while (!s.empty()) {
    auto len = min(kNumCols - current_col_, s.size());
    size_t endline_or_col_chars = s.find_first_of('\n', 0, len);
    bool endline_found = (endline_or_col_chars != npos);

    if (endline_or_col_chars == npos) {
      endline_or_col_chars = len;
    }
    auto first_num_col_chars = s.substr(0, endline_or_col_chars);

    PrintStringLineAtCursor(first_num_col_chars, color);

    if (endline_found && current_col_ != 0 && current_row_ < kNumRows) {
      // Fill remaining part as 0.
      for (size_t i = current_col_; i < kNumCols; i++) {
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

  auto* vga = reinterpret_cast<uint16_t*>(kVGAMemoryStart);
  for (size_t i = 0; i < current_row_; i++) {
    for (size_t j = 0; j < kNumCols; j++) {
      vga[i * kNumCols + j] = text_buffer_[i][j];
    }
  }
  if (current_col_ != 0) {
    for (size_t i = 0; i < kNumCols; i++) {
      vga[kNumCols * current_row_ + i] = text_buffer_[current_row_][i];
    }
  }
}

int VGAOutput::EstimateNumScrollDown(const std::vector<KeyStroke>& ks,
                                     int start, int end) {
  int total_scroll_down = 0, current_col = current_col_;
  for (int i = start; i < end; i++) {
    if (current_col == kNumCols) {
      total_scroll_down++;
      current_col = 0;
    }

    if (ks.at(i).c == KEY_CODES::ENTER) {
      total_scroll_down++;
      current_col = 0;
      continue;
    } else if (ks.at(i).c == KEY_CODES::BACKSPACE) {
      if (current_col > 0) {
        current_col--;
      }
      continue;
    }
    current_col++;
  }

  return total_scroll_down;
}

// Print char at (row, col)
void VGAOutput::PutCharAt(size_t row, size_t col, char c, VGAColor color) {
  // Carriage return should be treated as empty.
  if (c == 13) {
    c = 0;
  }

  text_buffer_[row][col] = (c | (color << 8));
}

void VGAOutput::PrintKeyStrokes(const std::vector<KeyStroke>& ks, int start,
                                int end) {
  if (start >= end) {
    return;
  }

  if (current_row_ == kNumRows) {
    ScrollTextBufferUp();
    current_row_--;
  }

  for (int i = start; i < end; i++) {
    if (current_col_ == kNumCols) {
      if (current_row_ < kNumRows - 1) {
        current_row_++;
      } else {
        ScrollTextBufferUp();
      }
      current_col_ = 0;
    }

    if (ks.at(i).c == KEY_CODES::ENTER) {
      if (current_row_ < kNumRows - 1) {
        current_row_++;
      } else {
        ScrollTextBufferUp();
      }
      current_col_ = 0;
      continue;
    } else if (ks.at(i).c == KEY_CODES::BACKSPACE) {
      if (current_col_ > 0) {
        current_col_--;
        PutCharAt(current_row_, current_col_, 0, VGAColor::White);
      }
      continue;
    } else if (ks.at(i).ToChar() == 0) {
      continue;
    } else {
      PutCharAt(current_row_, current_col_, ks.at(i).ToChar(), VGAColor::White);
    }
    current_col_++;
  }

  // Now flush.
  FlushTextBuffer();
}

// Scroll the text buffer up.
void VGAOutput::ScrollTextBufferUp(size_t num_up) {
  if (num_up >= kNumRows) {
    // Just clear entire buffer.
    for (size_t i = 0; i < kNumRows; i++) {
      for (size_t j = 0; j < kNumCols; j++) {
        text_buffer_[i][j] = 0;
      }
    }
    return;
  }

  for (size_t i = num_up; i < kNumRows; i++) {
    for (size_t j = 0; j < kNumCols; j++) {
      text_buffer_[i - num_up][j] = text_buffer_[i][j];
    }
  }

  // Clear the last num_up rows.
  for (size_t i = 1; i <= num_up; i++) {
    for (size_t j = 0; j < kNumCols; j++) {
      text_buffer_[kNumRows - i][j] = 0;
    }
  }
}

void VGAOutput::FlushTextBuffer() {
  auto* vga = reinterpret_cast<uint16_t*>(kVGAMemoryStart);

  for (size_t i = 0; i < kNumRows; i++) {
    for (size_t j = 0; j < kNumCols; j++) {
      vga[i * kNumCols + j] = text_buffer_[i][j];
    }
  }
}

void VGAOutput::ClearScreen() {
  auto* vga = reinterpret_cast<uint16_t*>(kVGAMemoryStart);

  for (size_t i = 0; i < kNumRows; i++) {
    for (size_t j = 0; j < kNumCols; j++) {
      vga[i * kNumCols + j] = text_buffer_[i][j] = 0;
    }
  }

  current_row_ = 0;
  current_col_ = 0;
}

}  // namespace Kernel
