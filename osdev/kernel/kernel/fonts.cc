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

  QemuSerialLog::Logf("font size : %x width : %d height : %d\n",
                      header->charsize, header->width, header->height);

  for (int f = 0; f < 512; f++) {
    uint8_t* font = data + header->headersize + header->charsize * (f);
    for (int i = 0; i < 16; i++) {
      for (int j = 0; j < 8; j++) {
        GraphicManager::Color color = 0;
        if (font[i] & (1 << j)) {
          color = 0xFFFFFF;
        }
        GraphicManager::GetGraphicManager().DrawAt(i + 8 * f / 800 * 16,
                                                   8 * f + 8 - j, color);
      }
    }
  }

  // Read table.
  /*
  uint32_t num_glyph = header->length;
  uint32_t len = 0;
  uint8_t* current =
      data + header->headersize + header->length * header->charsize;
  while (len < num_glyph) {
    uint8_t* line_end = current;
    QemuSerialLog::Logf("%d : ", len);
    while (*line_end != 0xFF) {
      QemuSerialLog::Logf("%x ", *line_end);
      line_end++;
    }
    QemuSerialLog::Logf("\n");
    current = line_end + 1;
    len++;
  }
  */
  font_loaded_ = true;
}

}  // namespace Kernel
