#ifndef FONTS_H
#define FONTS_H

#include "../std/hash_map.h"
#include "../std/stdint.h"

namespace Kernel {

class FontManager {
 public:
  static FontManager& GetFontManager() {
    static FontManager font_manager;
    return font_manager;
  }

  void Init();
  void Draw(int font, int row, int col);

 private:
  FontManager() = default;

  uint32_t height_;
  uint32_t width_;
  uint32_t charsize_;

  bool font_loaded_ = false;

  // Mapping between UTF-8 bytes to font index.
  std::HashMap<int, int> fonts_;

  std::vector<char*> fonts_data_;
};

}  // namespace Kernel

#endif
