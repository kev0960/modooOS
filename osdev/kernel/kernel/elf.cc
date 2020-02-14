#include "elf.h"
#include "../std/printf.h"
#include "kernel_util.h"
namespace Kernel {
namespace {

constexpr uint32_t kELFMagic = 0x464c457f;
constexpr uint8_t kELFClass64Bit = 0x2;
constexpr uint8_t kELFLittleEndian = 0x1;

}  // namespace

ELFReader::ELFReader(const uint8_t* data, size_t file_size) {
  static_assert(sizeof(ELFHeader) == 0x40);
  static_assert(sizeof(ELFProgramHeader) == 0x38);
  static_assert(sizeof(ELFSectionHeader) == 0x40);
  ASSERT(file_size >= 0x40);

  header_ = *reinterpret_cast<const ELFHeader*>(data);

  // First check the magic.
  if (header_.e_ident_magic != kELFMagic) {
    error_ = "ELF Magic does not match.";
    is_valid_ = false;
    return;
  } else {
    is_valid_ = true;
  }

  if (header_.e_ident_class != kELFClass64Bit) {
    error_ = "Not a 64bit ELF file.";
    is_valid_ = false;
    return;
  }

  if (header_.e_ident_data != kELFLittleEndian) {
    error_ = "Not a Little endian encoded file.";
    is_valid_ = false;
    return;
  }

  program_headers_.reserve(header_.e_phnum);
  for (int ph_off = 0; ph_off < header_.e_phnum; ph_off++) {
    program_headers_.push_back(*reinterpret_cast<const ELFProgramHeader*>(
        data + header_.e_phoff + ph_off * sizeof(ELFProgramHeader)));
  }

  section_headers_.reserve(header_.e_shnum);
  for (int sh_off = 0; sh_off < header_.e_shnum; sh_off++) {
    program_headers_.push_back(*reinterpret_cast<const ELFProgramHeader*>(
        data + header_.e_shoff + sh_off * sizeof(ELFSectionHeader)));
  }
}

}  // namespace Kernel

