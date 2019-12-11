#include <stdint.h>

#define GDT_TABLE_SIZE 0x800

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
  uint32_t gdt_table;  // Pointer to the first GDT table;
};

typedef struct GDTEntry GDTEntry;
typedef struct GDTEntryPtr GDTEntryPtr;

void SetUpGDTTables(GDTEntryPtr* ptr, GDTEntry* gdt_table_start);
