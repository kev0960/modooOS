#ifndef ACPI_H
#define ACPI_H

#include "../std/printf.h"
#include "../std/string.h"
#include "paging.h"
#include "vm.h"

namespace Kernel {

class ACPIManager {
 public:
  static ACPIManager& GetACPIManager() {
    static ACPIManager acpi_manager;
    return acpi_manager;
  }

  struct RSDPDescriptor {
    char signature[8];
    uint8_t checksum;
    char oemid[6];
    uint8_t revision;
    uint32_t rsdt_addr;
  } __attribute__((packed));

  // This is version 2.0 of RSDP. This fully contains the previous version.
  struct RSDPDescriptor2 {
    RSDPDescriptor desc;
    uint32_t len;
    uint64_t xsdt_addr;
    uint8_t extended_checksum;
    uint8_t reserved[3];
  } __attribute__((packed));

  struct ACPISDTHeader {
    char signature[4];
    uint32_t len;
    uint8_t revision;
    uint8_t checksum;
    char oem_id[6];
    char oem_table_id[8];
    uint32_t oem_revision;
    uint32_t creator_id;
    uint32_t creator_revision;
  } __attribute__((packed));

  struct ACPITableEntry {
    ACPISDTHeader header;
    uint8_t* data;
  };

  // Find "RSD PTR " string in the memory region 0xE0000 ~ 0xFFFFF.
  // The string is always on 16byte boundary.
  void DetectRSDP() {
    for (size_t i = 0xE0000; i < 0xFFFFF; i += 16) {
      char* str = PhysToKernelVirtual<size_t, char*>(i);
      if (strncmp("RSD PTR ", str, 8) == 0) {
        RSDPDescriptor* desc = reinterpret_cast<RSDPDescriptor*>(str);
        ASSERT(desc->revision == 0);

        if (desc->revision == 0) {
          desc_ = desc;
        }

        kprintf("Chksum : %d ", VerifyChecksum(desc));

        break;
      }
    }
  }

  void ParseRSDT();

  ACPITableEntry* GetEntry(const char* entry_name) {
    for (auto& entry : entries_) {
      if (strncmp(entry_name, entry.header.signature, 4) == 0) {
        return &entry;
      }
    }
    return nullptr;
  }

  void ParseMADT();

  void EnableACPI();

  void ListTables() {
    for (auto& entry : entries_) {
      for (int i = 0; i < 4; i++) {
        kprintf("%c", entry.header.signature[i]);
      }
      kprintf("\n");
    }
  }

  template <typename T>
  bool VerifyChecksum(const T* data) {
    char total = 0;
    for (size_t i = 0; i < sizeof(T); i++) {
      total += reinterpret_cast<const char*>(data)[i];
    }

    return total == 0;
  }

  bool VerifyHeaderChecksum(const ACPISDTHeader* header) {
    uint8_t sum = 0;
    for (uint32_t i = 0; i < header->len; i++) {
      sum += reinterpret_cast<const uint8_t*>(header)[i];
    }
    return sum == 0;
  }

  std::vector<uint8_t>& GetCoreAPICIds() { return core_apic_ids_; }

  ACPIManager(const ACPIManager&) = delete;
  ACPIManager& operator=(const ACPIManager&) = delete;

 private:
  ACPIManager() = default;

  RSDPDescriptor* desc_;

  uint32_t* sdts_;
  uint32_t num_sdt_;

  std::vector<ACPITableEntry> entries_;
  std::vector<uint8_t> core_apic_ids_;
};
}  // namespace Kernel

#endif
