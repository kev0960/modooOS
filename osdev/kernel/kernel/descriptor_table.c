#include "descriptor_tables.h"

#define NUM_GDT_ENTRY_DEFINED 5

void SetUpGDTTableEntry(GDTEntry* entry, uint32_t base_addr, uint32_t limit,
                        uint8_t access, uint8_t granularity);

void SetUpGDTTables(GDTEntryPtr* ptr, GDTEntry* gdt_table_start) {
  // Null Data Segment
  SetUpGDTTableEntry(gdt_table_start, 0, 0, 0, 0);

  // Kernel Code Segment
  // access bit : P DPL 1 1 C R A
  //              1  00 1 1 0 1 0
  // granularity: G D L AVL
  //              1 0 1  0
  // (Note D must be 0 when L = 1)
  SetUpGDTTableEntry(gdt_table_start + 1, 0, 0xFFFFFFFF, /* access */ 0b10011010,
                     /* granularity */ 0b10100000);
  // Kernel Data Segment
  SetUpGDTTableEntry(gdt_table_start + 2, 0, 0xFFFFFFFF, /* access */ 0b10010010,
                     /* granularity */ 0b10100000);

  // User Code Segment (DPL --> 11)
  SetUpGDTTableEntry(gdt_table_start + 3, 0, 0xFFFFFFFF, /* access */ 0b11111010,
                     /* granularity */ 0b10100000);
  // User Data Segment
  SetUpGDTTableEntry(gdt_table_start + 4, 0, 0xFFFFFFFF, /* access */ 0b11110010,
                     /* granularity */ 0b10100000);


  ptr->limit = (sizeof(GDTEntry) * NUM_GDT_ENTRY_DEFINED) - 1;
  ptr->gdt_table = (uint32_t)gdt_table_start;
}

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

