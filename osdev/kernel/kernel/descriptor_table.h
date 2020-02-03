#include "../std/types.h"
#include "cpu.h"

namespace Kernel {

struct GDTEntry {
  uint16_t segment_limit_low;
  uint16_t base_addr_low;  // [0:15]
  uint8_t base_addr_mid;   // [16:23]
  uint8_t access_flag;
  uint8_t granularity;
  uint8_t base_addr_high;  // [23:32]
} __attribute__((packed));

struct GDTEntryPtr {
  uint16_t limit;      // GDT_TABLE_SIZE - 1
  uint64_t gdt_table;  // Pointer to the first GDT table;
} __attribute__((packed));

class GDTTableManager {
 public:
  static GDTTableManager& GetGDTTableManager() {
    static GDTTableManager gdt_table_manager;
    return gdt_table_manager;
  }

  void SetUpGDTTables();

  static constexpr size_t kNumGDTEntryDefined = 5;
  static constexpr size_t kGDTTableSize = 0x800;

 private:
  GDTTableManager() = default;

  GDTEntryPtr gdt_entry_ptr_;
  GDTEntry gdt_entries_[kGDTTableSize];
};

}  // namespace Kernel
