#ifndef GRAPHIC_H
#define GRAPHIC_H

#include "../std/stdint.h"
#include "../std/string_view.h"
#include "keyboard.h"
#include "sync.h"

namespace Kernel {

void ParseMultibootInfo(void* multiboot_info);

class GraphicManager {
 public:
  static GraphicManager& GetGraphicManager() {
    static GraphicManager graphic_manager;
    return graphic_manager;
  }

  using Color = uint32_t;

  bool IsReady() const { return is_ready_; }

  void Init(int width, int height, int pixel_size, uint32_t* video_mem);

  void DrawAt(int row, int col, Color color);

  // Possibly UTF-8 encoded character.
  void PutChar(int c, Color color = 0xFFFFFF);
  void PutCharAt(int c, int row, int col);

  void PrintString(std::string_view s, Color color = 0xFFFFFF);
  void PrintAnchorString(std::string_view s, Color color = 0xFFFFFF);
  void PrintKeyStrokes(const std::vector<KeyStroke>& ks, int start, int end);

  bool ShouldSync() const { return !is_synced_; }

  struct FrameBufferInfo {
    // Location to draw at.
    int screen_row;
    int screen_col;

    // Size of the buffer.
    int buffer_width;
    int buffer_height;
  };
  void SyncScreenWith(uint32_t* buffer, FrameBufferInfo* info);
  void SyncScreen();

  void PrintLock();
  void PrintUnlock();

  void SetAnchor(int anchor_row, int anchor_col) {
    anchor_row_ = anchor_row;
    anchor_col_ = anchor_col;
  }

  int GetWidth() const { return width_; }
  int GetHeight() const { return height_; }
  int GetPixelSize() const { return pixel_size_; }

 private:
  GraphicManager() = default;

  void MoveCursor();
  void Scroll();
  void Backspace();

  bool is_ready_ = false;

  int width_;
  int height_;
  int pixel_size_;

  // Actual video memory.
  uint32_t* video_mem_ = nullptr;

  // The last synced version of the video memory.
  // Anything that is different between last_sync and buffer will be copied to
  // video mem.
  uint32_t* last_sync_ = nullptr;
  bool is_synced_ = false;

  // Current video buffer.
  uint32_t* buffer_ = nullptr;

  // Location of the current cursor.
  int cur_row_;
  int cur_col_;

  // TODO Fetch those value from the actual font file.
  const int font_width = 8;
  const int font_height = 16;
  const int font_margin_right = 1;
  const int font_margin_bottom = 1;

  SpinLockNoLockInIntr print_lock_;

  // If we set the anchor in the screen, then cursor cannot move pass this
  // anchor.
  int anchor_row_ = 0;
  int anchor_col_ = 0;
};

}  // namespace Kernel

#endif
