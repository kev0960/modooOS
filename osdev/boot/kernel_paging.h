#ifndef KERNEL_PAGING_H
#define KERNEL_PAGING_H

#define KERNEL_VIRTUAL_START  0xFFFFFFFF80400000
#define KERNEL_PHYSICAL_START 0x0000000000400000

#define PML4_ADDR_TO_ENTRY_INDEX(addr) (((addr) >> 39) & 0x1FF)
#define PDP_ADDR_TO_ENTRY_INDEX(addr) (((addr) >> 30) & 0x1FF)

#define ONE_GB (1 << 30)
#define TWO_MB (1 << 21)

#define PAGE_TABLE_SIZE (1 << 12)  // 8 bytes per entry, 512 entries.

#define PAGE_ENTRY_SIZE (1 << 3)  // 8 bytes
#define NUM_PAGE_ENTRY (1 << 9)

#define PAGE_TABLE_ALIGNMENT 0x1000
#define PAGE_ENTRY_PRESENT 0x1  // Bit 0
#define PAGE_ENTRY_RW 0x2       // Bit 1
#define PAGE_PAGE_SIZE \
  (1 << 7)  // For 2MB paging, must be set on PDE (also the CR4.PAE)

#define KERNEL_BOOT_STACK_SIZE 0x4000
#define KERNEL_BOOT_STACK_ALIGN 0x1000
#define KERNEL_CR4 (1 << 5)  // PAE

#define CONTROL_REGISTER0_PROTECTED_MODE_ENABLED (1 << 0)
#define CONTROL_REGISTER0_EXTENSION_TYPE (1 << 4)
#define CONTROL_REGISTER0_PAGE (1 << 31)

#define KERNEL_CR0                                                     \
  (CONTROL_REGISTER0_PAGE | CONTROL_REGISTER0_PROTECTED_MODE_ENABLED | \
   CONTROL_REGISTER0_EXTENSION_TYPE)

#define MSR_EFER 0xC0000080
#define MSR_EFER_LME (1 << 8)

#endif
