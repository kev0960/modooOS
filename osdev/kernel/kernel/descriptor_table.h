#ifndef DESCRIPTOR_TABLE
#define DESCRIPTOR_TABLE

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

  // (+2) for TSS segment descriptor.
  static constexpr size_t kNumGDTEntryDefined = 5 + 2;

 private:
  GDTTableManager() = default;

  GDTEntryPtr gdt_entry_ptr_;
  GDTEntry gdt_entries_[kNumGDTEntryDefined];
};

struct TaskSegmentDescriptor {
  uint16_t segment_limit_low;
  uint16_t base_addr_low;  // [0:15]
  uint8_t base_addr_mid;   // [16:23]
  uint8_t access_flag;
  uint8_t granularity;
  uint8_t base_addr_high;  // [23:32]
  uint32_t base_half;      // [32:63]
  uint32_t zero;           // MBZ
} __attribute__((packed));

struct TSS {
  /*
  uint32_t reserved;
  uint32_t rsp0_low;
  uint32_t rsp0_high;
  uint32_t rsp1_low;
  uint32_t rsp1_high;
  uint32_t rsp2_low;
  uint32_t rsp2_high;
  uint64_t reserved2;
  uint32_t ist1_low;
  uint32_t ist1_high;
  uint32_t ist2_low;
  uint32_t ist2_high;
  uint32_t ist3_low;
  uint32_t ist3_high;
  uint32_t ist4_low;
  uint32_t ist4_high;
  uint32_t ist5_low;
  uint32_t ist5_high;
  uint32_t ist6_low;
  uint32_t ist6_high;
  uint32_t ist7_low;
  uint32_t ist7_high;
  uint64_t reserved3;
  uint32_t iomap_base_addr;
  uint32_t io_permission_bitmap;*/
  uint32_t arr[26];
};

class TaskStateSegmentManager {
 public:
  static TaskStateSegmentManager& GetTaskStateSegmentManager() {
    static TaskStateSegmentManager task_state_segment_manager;
    return task_state_segment_manager;
  }

  void SetUpTaskStateSegments();
  TSS* GetTSS() { return &tss_; }

  void SetRSP0(uint64_t rsp) {
    // tss_.rsp0_low = rsp;
    // tss_.rsp0_high = rsp >> 32;
    tss_.arr[1] = rsp;
    tss_.arr[2] = rsp >> 32;
  }

  void LoadTR();

 private:
  TaskStateSegmentManager() = default;

  TSS tss_;
};

constexpr static uint16_t kKernelCodeSegment = 0x8;

}  // namespace Kernel

#endif
