#ifndef GRAPHIC_H
#define GRAPHIC_H

#include "../std/stdint.h"

namespace Kernel {

void ParseMultibootInfo(void* multiboot_info);

class GraphicManager {
 public:
  static GraphicManager& GetGraphicManager() {
    static GraphicManager graphic_manager;
    return graphic_manager;
  }

  using Color = uint32_t;

  void Init(int width, int height, int pixel_size, uint32_t* video_mem);

  void DrawAt(int row, int col, Color color);

 private:
  GraphicManager() = default;

  void ReadFonts();

  int width_;
  int height_;
  int pixel_size_;

  uint32_t* video_mem_ = nullptr;
  uint32_t* buffer_ = nullptr;
};

}  // namespace Kernel

#endif
