#include "fonts.h"

#include "fs/ext2.h"
#include "graphic.h"
#include "kmalloc.h"

namespace Kernel {

constexpr char kFontFilePath[] =
    "/usr/share/consolefonts/ter-powerline-v16v.psf";

struct PSF2Header {
  uint32_t magic;
  uint32_t version;
  uint32_t headersize; /* offset of bitmaps in file */
  uint32_t flags;
  uint32_t length;        /* number of glyphs */
  uint32_t charsize;      /* number of bytes for each character */
  uint32_t height, width; /* max dimensions of glyphs */
                          /* charsize = height * ((width + 7) / 8) */
};

void FontManager::Init() {
  // Load font.
  auto& ext2 = Ext2FileSystem::GetExt2FileSystem();
  auto font_file_info = ext2.Stat(kFontFilePath);
  if (font_file_info.inode == 0) {
    QemuSerialLog::Logf("Font file %s does not exist! \n", kFontFilePath);
    return;
  }

  uint8_t* data = (uint8_t*)kmalloc(font_file_info.file_size);
  ext2.ReadFile(kFontFilePath, data, font_file_info.file_size);

  PSF2Header* header = (PSF2Header*)(data);
  if (header->magic != 0x864ab572 || header->headersize != 32) {
    return;
  }

  if (header->flags == 1) {
    QemuSerialLog::Logf("Has unicode table\n");
  }

  height_ = header->height;
  width_ = header->width;
  charsize_ = header->charsize;

  QemuSerialLog::Logf("font size : %x width : %d height : %d\n",
                      header->charsize, header->width, header->height);

  fonts_data_.reserve(header->length);
  for (size_t i = 0; i < header->length; i++) {
    fonts_data_.push_back((char*)kmalloc(header->charsize));

    uint8_t* font = data + header->headersize + header->charsize * i;
    for (size_t j = 0; j < header->charsize; j++) {
      fonts_data_.back()[j] = font[j];
    }
  }

  // Read Unicode table.
  uint32_t num_glyph = header->length;
  uint32_t len = 0;
  uint8_t* current =
      data + header->headersize + header->length * header->charsize;
  while (len < num_glyph) {
    uint8_t* line_end = current;
    while (*line_end != 0xFF) {
      int ch = 0;
      if (*line_end <= 0b01111111) {
        ch = *line_end;
        line_end++;
      } else if (*line_end <= 0b11011111) {
        ch = (*line_end++ << 8);
        ch |= (*line_end++);
      } else if (*line_end <= 0b11101111) {
        ch = (*line_end++ << 16);
        ch |= (*line_end++ << 8);
        ch |= (*line_end++);
      } else {
        ch = (*line_end++ << 24);
        ch |= (*line_end++ << 16);
        ch |= (*line_end++ << 8);
        ch |= (*line_end++);
      }
      fonts_.insert(ch, len);
    }
    current = line_end + 1;
    len++;
  }

  font_loaded_ = true;
}

void FontManager::Draw(int font, int row, int col) {
  const int* pos = fonts_.find(font);
  if (pos == nullptr) {
    return;
  }

  char* font_data = fonts_data_[*pos];
  for (size_t i = 0; i < height_; i++) {
    for (size_t j = 0; j < width_; j++) {
      GraphicManager::Color color = 0;
      if (font_data[i] & (1 << j)) {
        color = 0xFFFFFF;
      }
      GraphicManager::GetGraphicManager().DrawAt(i + row, col + 8 - j, color);
    }
  }
}

}  // namespace Kernel
