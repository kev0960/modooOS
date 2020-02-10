#ifndef ELF_H
#define ELF_H

#include "../std/stdint.h"
#include "../std/string.h"
#include "../std/types.h"

namespace Kernel {

struct ELFHeader {
  uint32_t e_ident_magic;
  uint8_t e_ident_class;
  uint8_t e_ident_data;
  uint8_t e_ident_version;
  uint8_t e_ident_osabi;
  uint8_t e_ident_abiversion;
  uint8_t e_ident_pad[7];  // Unused.
  uint16_t e_type;
  uint16_t e_machine;
  uint32_t e_version;
  uint64_t e_entry;
  uint64_t e_phoff;
  uint64_t e_shoff;
  uint32_t e_flags;
  uint16_t e_ehsize;
  uint16_t e_phentsize;
  uint16_t e_phnum;
  uint16_t e_shentsize;
  uint16_t e_shnum;
  uint16_t e_shstrndx;
} __attribute__((packed));  // Must be 0x40 bytes.

struct ELFProgramHeader {
  uint32_t p_type;
  uint32_t p_flags;
  uint64_t p_offset;
  uint64_t p_vaddr;
  uint64_t p_paddr;
  uint64_t p_filesz;
  uint64_t p_memsz;
  uint64_t p_align;
} __attribute__((packed));  // Must be 0x38 bytes.

struct ELFSectionHeader {
  uint32_t sh_name;
  uint32_t sh_type;
  uint64_t sh_flags;
  uint64_t sh_addr;
  uint64_t sh_offset;
  uint64_t sh_size;
  uint32_t sh_link;
  uint32_t sh_info;
  uint64_t sh_addralign;
  uint64_t sh_entsize;
} __attribute__((packed));  // Must be 0x40 bytes.

class ELFReader {
 public:
  ELFReader(const uint8_t* data, size_t file_size);

  bool IsValid() const { return is_valid_; }
  KernelString Error() const { return error_; }

  const ELFHeader& GetHeader() const { return header_; }
  const std::vector<ELFProgramHeader>& GetProgramHeaders() const {
    return program_headers_;
  }

  const std::vector<ELFSectionHeader>& GetSectionHeaders() const {
    return section_headers_;
  }

 private:
  bool is_valid_;
  KernelString error_;

  ELFHeader header_;
  std::vector<ELFProgramHeader> program_headers_;
  std::vector<ELFSectionHeader> section_headers_;
};

}  // namespace Kernel

#endif
