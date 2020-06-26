#ifndef FONTS_H
#define FONTS_H

#include "../std/map.h"
#include "../std/stdint.h"

namespace Kernel {

class FontManager {
 public:
  static FontManager& GetFrontManager() {
    static FontManager font_manager;
    return font_manager;
  }

  void Init();

 private:
  FontManager() = default;

  uint32_t height_;
  uint32_t width_;

  bool font_loaded_ = false;

  char* char_to_font_[128];
};

}  // namespace Kernel

#endif
