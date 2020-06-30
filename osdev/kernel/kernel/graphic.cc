#include "graphic.h"

#include "../boot/multiboot2.h"
#include "fonts.h"
#include "kmalloc.h"
#include "paging.h"
#include "qemu_log.h"
#include "scheduler.h"

namespace Kernel {
namespace {

uint64_t RoundUpToFourKB(uint64_t sz) {
  constexpr uint64_t kFourKB = (1 << 12);
  if (sz % kFourKB == 0) {
    return sz;
  }
  return (sz / kFourKB + 1) * kFourKB;
}

}  // namespace

void ParseMultibootInfo(void* multiboot_info) {
  uint8_t* info_addr = reinterpret_cast<uint8_t*>(
      PageTableManager::kKernelVMStart + (uint64_t)(multiboot_info));
  QemuSerialLog::Logf("info addr : %lx\n", info_addr);
  QemuSerialLog::Logf("reserved : %lx\n", *(uint32_t*)(info_addr + 4));

  uint32_t total_size = *(uint32_t*)info_addr;
  QemuSerialLog::Logf("total_size %lx\n", *(uint32_t*)info_addr);

  uint32_t index = 8;
  while (index < total_size) {
    uint32_t tag_type = *(uint32_t*)(info_addr + index);
    if (tag_type == MULTIBOOT_TAG_TYPE_END) {
      break;
    }
    if (tag_type == MULTIBOOT_TAG_TYPE_FRAMEBUFFER) {
      multiboot_tag_framebuffer_common* common =
          (multiboot_tag_framebuffer_common*)(info_addr + index);
      GraphicManager::GetGraphicManager().Init(
          common->framebuffer_width, common->framebuffer_height,
          common->framebuffer_bpp, (uint32_t*)common->framebuffer_addr);
    }

    uint32_t tag_size = *(uint32_t*)(info_addr + index + 4);
    // QemuSerialLog::Logf("Type : %d \n", tag_type);
    // QemuSerialLog::Logf("size : %d \n", tag_size);
    index += tag_size;
    if (index % 8 != 0) {
      index += (8 - index % 8);
    }
  }
}

void GraphicManager::Init(int width, int height, int pixel_size,
                          uint32_t* video_mem_phys) {
  QemuSerialLog::Logf("width : %d height %d pixel_size : %d vmem : %lx", width,
                      height, pixel_size, video_mem_phys);
  width_ = width;
  height_ = height;
  pixel_size_ = pixel_size;

  uint64_t video_mem_size = RoundUpToFourKB(width * height * (pixel_size_ / 8));

  // Now allocate the memory.
  video_mem_ = (uint32_t*)kaligned_alloc((1 << 12), video_mem_size);

  // Map page table.
  PageTableManager::GetPageTableManager().AllocateKernelPage(
      (uint64_t)video_mem_, video_mem_size, (uint64_t)video_mem_phys);

  last_sync_ = (uint32_t*)kmalloc(width_ * height_ * (pixel_size_ / 8));
  buffer_ = (uint32_t*)kmalloc(width_ * height_ * (pixel_size_ / 8));

  KernelThread* sync_thread = new KernelThread([] {
    auto& m = GraphicManager::GetGraphicManager();
    while (1) {
      if (m.ShouldSync()) {
        GraphicManager::GetGraphicManager().SyncScreen();
      }
      KernelThreadScheduler::GetKernelThreadScheduler().Yield();
    }
  });
  sync_thread->Start();

  is_ready_ = true;
}

void GraphicManager::DrawAt(int row, int col, Color color) {
  if (video_mem_ == nullptr) {
    return;
  }

  video_mem_[width_ * row + col] = color;
}

void GraphicManager::PutChar(int c, Color color) {
  if (c == '\n') {
    cur_col_ = 0;

    const int font_actual_height = font_height + font_margin_bottom;
    cur_row_ += font_actual_height;
    if (cur_row_ + font_actual_height >= height_) {
      Scroll();
      cur_row_ -= font_actual_height;

      anchor_row_--;

      // If the anchor goes out of the screen.
      if (anchor_row_ < 0) {
        anchor_row_ = 0;
        anchor_col_ = 0;
      }
    }

    is_synced_ = false;
    return;
  }

  char* font = FontManager::GetFontManager().GetFont(c);
  if (font == nullptr) {
    MoveCursor();
    return;
  }

  for (int i = 0; i < font_height; i++) {
    for (int j = 0; j < font_width; j++) {
      GraphicManager::Color real_color = 0;
      if (font[i] & (1 << j)) {
        real_color = color;
      }
      buffer_[width_ * (i + cur_row_) + cur_col_ + 8 - j] = real_color;
    }
  }
  is_synced_ = false;
  MoveCursor();
}

void GraphicManager::PrintString(std::string_view s, Color color) {
  for (size_t i = 0; i < s.size(); i++) {
    PutChar(s[i], color);
  }
}

void GraphicManager::PrintAnchorString(std::string_view s, Color color) {
  for (size_t i = 0; i < s.size(); i++) {
    PutChar(s[i], color);
  }

  anchor_col_ = cur_col_;
  anchor_row_ = cur_row_;
  QemuSerialLog::Logf("anchor : %d %d \n", anchor_col_, anchor_row_);
}

void GraphicManager::Scroll() {
  const int scroll = font_height + font_margin_bottom;
  // Scroll Everything up by (font_height + font_margin_bottom).
  for (int i = scroll; i < height_; i++) {
    for (int j = 0; j < width_; j++) {
      buffer_[(i - scroll) * width_ + j] = buffer_[i * width_ + j];
    }
  }

  // Fill the bottom as empty.
  for (int i = height_ - scroll; i < height_; i++) {
    for (int j = 0; j < width_; j++) {
      buffer_[i * width_ + j] = 0;
    }
  }
}

void GraphicManager::MoveCursor() {
  cur_col_ += (font_width + font_margin_right);
  if (cur_col_ >= width_) {
    cur_col_ = 0;
    cur_row_ += (font_height + font_margin_bottom);
  }

  const int scroll = font_height + font_margin_bottom;
  if (cur_row_ + scroll >= height_) {
    Scroll();

    cur_row_ -= scroll;
    anchor_row_--;

    // If the anchor goes out of the screen.
    if (anchor_row_ < 0) {
      anchor_row_ = 0;
      anchor_col_ = 0;
    }
  }
}

void GraphicManager::SyncScreen() {
  is_synced_ = true;

  for (int i = 0; i < height_ * width_; i++) {
    // We only copy the part that is different to last synced buffer.
    // This is because reading video memory is super small and we don't want to
    // copy entire buffer to video memory all the time.
    if (last_sync_[i] != buffer_[i]) {
      last_sync_[i] = buffer_[i];
      video_mem_[i] = buffer_[i];
    }
  }
}

void GraphicManager::SyncScreenWith(uint32_t* buffer, FrameBufferInfo* info) {
  for (int i = 0; i < info->buffer_height; i++) {
    for (int j = 0; j < info->buffer_width; j++) {
      int screen_index =
          (i + info->screen_row) * width_ + (j + info->screen_col);
      int buffer_index = i * info->buffer_width + j;
      if (last_sync_[screen_index] != buffer[buffer_index]) {
        buffer_[screen_index] = buffer[buffer_index];
        last_sync_[screen_index] = buffer_[screen_index];
        video_mem_[screen_index] = buffer_[screen_index];
      }
    }
  }
  is_synced_ = true;
}

void GraphicManager::Backspace() {
  QemuSerialLog::Logf("backsp %d %d %d %d \n", cur_row_, cur_col_, anchor_row_,
                      anchor_col_);
  const int font_actual_width = font_width + font_margin_right;
  cur_col_ -= font_actual_width;
  if (anchor_col_ > cur_col_ && anchor_row_ >= cur_row_) {
    cur_col_ += font_actual_width;
    return;
  }

  // Then we have to go to the upper row.
  if (cur_col_ < 0) {
    cur_row_ -= (font_height + font_margin_bottom);
    if (cur_row_ < 0) {
      cur_row_ = 0;
    }

    cur_col_ = (width_ / font_actual_width - 1) * font_actual_width;
  }

  // Erase Current line.
  for (int i = cur_row_; i < cur_row_ + font_height + font_margin_bottom; i++) {
    for (int j = cur_col_; j < cur_col_ + font_actual_width; j++) {
      buffer_[i * width_ + j] = 0;
    }
  }
  is_synced_ = false;
}

void GraphicManager::PrintKeyStrokes(const std::vector<KeyStroke>& ks,
                                     int start, int end) {
  for (int i = start; i < end; i++) {
    if (ks.at(i).c == KEY_CODES::BACKSPACE) {
      Backspace();
    } else if (ks.at(i).c == KEY_CODES::ENTER) {
      PutChar('\n');
    } else if (ks.at(i).ToChar() != 0) {
      PutChar(ks.at(i).ToChar());
    }
  }
}

void GraphicManager::PrintLock() { print_lock_.lock(); }
void GraphicManager::PrintUnlock() { print_lock_.unlock(); }

}  // namespace Kernel
