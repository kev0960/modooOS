#include "descriptor_table.h"

#include "../std/printf.h"
#include "cpu_context.h"

namespace Kernel {
namespace {

void SetUpGDTTableEntry(GDTEntry* entry, uint32_t base_addr, uint32_t limit,
                        uint8_t access, uint8_t granularity) {
  entry->base_addr_low = base_addr & 0xFFFF;         // Last 16 bits.
  entry->base_addr_mid = (base_addr >> 16) & 0xFF;   // Middle 8 bits
  entry->base_addr_high = (base_addr >> 24) & 0xFF;  // High 8 bits.

  entry->segment_limit_low = limit & 0xFFFF;  // Last 16 bits
  entry->granularity = (limit >> 16) & 0x0F;  // 4 bits of middle

  entry->access_flag = access;
  entry->granularity |= (granularity & 0xF0);
}

void SetTSSDescripter(TaskSegmentDescriptor* entry, uint64_t base_addr,
                      uint32_t limit, uint8_t access, uint8_t granularity) {
  entry->base_addr_low = base_addr & 0xFFFF;         // Last 16 bits.
  entry->base_addr_mid = (base_addr >> 16) & 0xFF;   // Middle 8 bits
  entry->base_addr_high = (base_addr >> 24) & 0xFF;  // High 8 bits.
  entry->base_half = (base_addr >> 32);

  entry->segment_limit_low = limit & 0xFFFF;  // Last 16 bits
  entry->granularity = (limit >> 16) & 0x0F;  // 4 bits of middle

  entry->access_flag = access;
  entry->granularity |= (granularity & 0xF0);
}
}  // namespace

void GDTTableManager::SetUpGDTTables() {
  ASSERT(gdt_tables_.size() == CPUContextManager::GetCurrentCPUId());

  GDTEntry* current_entries = reinterpret_cast<GDTEntry*>(
      kmalloc(sizeof(GDTEntry) * kNumGDTEntryDefined));

  descriptors_per_core_.push_back(current_entries);

  // Null Data Segment
  SetUpGDTTableEntry(&current_entries[0], 0, 0, 0, 0);

  // Kernel Code Segment
  // access bit : P DPL 1 1 C R A
  //              1  00 1 1 0 1 0
  // granularity: G D L AVL
  //              1 0 1  0
  // (Note D must be 0 when L = 1)
  SetUpGDTTableEntry(&current_entries[1], 0, 0xFFFFFFFF,
                     /* access */ 0b10011010,
                     /* granularity */ 0b10100000);
  // Kernel Data Segment
  SetUpGDTTableEntry(&current_entries[2], 0, 0xFFFFFFFF,
                     /* access */ 0b10010010,
                     /* granularity */ 0b10100000);

  // User Data Segment
  SetUpGDTTableEntry(&current_entries[3], 0, 0xFFFFFFFF,
                     /* access */ 0b11110010,
                     /* granularity */ 0b10100000);

  // User Code Segment (DPL --> 11)
  SetUpGDTTableEntry(&current_entries[4], 0, 0xFFFFFFFF,
                     /* access */ 0b11111010,
                     /* granularity */ 0b10100000);

  auto& task_state_segment_manager =
      TaskStateSegmentManager::GetTaskStateSegmentManager();

  // Create TSS.
  task_state_segment_manager.SetUpTaskStateSegments();

  SetTSSDescripter(
      reinterpret_cast<TaskSegmentDescriptor*>(&current_entries[5]),
      (uint64_t)task_state_segment_manager.GetTSS(), sizeof(TSS),
      /*access=*/0b10001001,
      /*granularity=*/0b10100000);

  gdt_tables_.push_back(
      reinterpret_cast<GDTEntryPtr*>(kmalloc(sizeof(GDTEntryPtr))));

  GDTEntryPtr* gdt_entry_ptr = gdt_tables_.back();
  gdt_entry_ptr->limit = sizeof(GDTEntry) * kNumGDTEntryDefined - 1;
  gdt_entry_ptr->gdt_table = reinterpret_cast<uint64_t>(current_entries);

  asm volatile("lgdt %0" ::"m"(*gdt_entry_ptr) :);

  asm volatile ("" : : : "memory");
  task_state_segment_manager.LoadTR();
}

void TaskStateSegmentManager::LoadTR() {
  // Load 6th entry of GDT.
  asm volatile(
      "movw $0x28, %%ax\n"
      "ltr %%ax" ::
          : "%ax");
}

void TaskStateSegmentManager::SetUpTaskStateSegments() {
  ASSERT(tss_.size() == CPUContextManager::GetCurrentCPUId());

  tss_.push_back(reinterpret_cast<TSS*>(kmalloc(sizeof(TSS))));
}

TSS* TaskStateSegmentManager::GetTSS() {
  ASSERT(tss_.size() > CPUContextManager::GetCurrentCPUId());
  return tss_[CPUContextManager::GetCurrentCPUId()];
}

void TaskStateSegmentManager::SetRSP0(uint64_t rsp) {
  TSS* current_tss = GetTSS();
  // tss_.rsp0_low = rsp;
  // tss_.rsp0_high = rsp >> 32;
  current_tss->arr[1] = rsp;
  current_tss->arr[2] = rsp >> 32;
}

}  // namespace Kernel
