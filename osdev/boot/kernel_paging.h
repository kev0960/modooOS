#define KERNEL_VIRTUAL_START 0xFFFF800000000000
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

#define GDT_TABLE_SIZE 0x800
#define GDT_ENTRY_SIZE 8

#define GDT_FLAG_FOUR_KILOBYTE_GRANULARITY (1 << 3)
#define GDT_FLAG_32BIT_PROTECTED_MODE (1 << 2)
#define GDT_FLAG_64BIT_MODE (1 << 1)

#define GDT_ACCESS_PRESENT (1 << 7)
#define GDT_ACCESS_PRIVILEGE_RING0 (0x0 << 5)
#define GDT_ACCESS_PRIVILEGE_RING1 (0x1 << 5)
#define GDT_ACCESS_PRIVILEGE_RING2 (0x2 << 5)
#define GDT_ACCESS_PRIVILEGE_RING3 (0x3 << 5)
#define GDT_ACCESS_EXECUTABLE (1 << 3)
#define GDT_ACCESS_DIRECTION_DOWN (1 << 2)
#define GDT_ACCESS_READABLE_WRITABLE (1 << 1)

#define DECLARE_GDT_ENTRY(base, limit, flags, access)                      \
  ((((((base)) >> 24) & 0xFF) << 56) | ((((flags)) & 0xF) << 52) |         \
   (((((limit)) >> 16) & 0xF) << 48) |                                     \
   (((((access) | (1 << 4))) & 0xFF) << 40) | ((((base)) & 0xFFF) << 16) | \
   (((limit)) & 0xFFFF))

#define GDT_NULL_DESCRIPTOR 0

#define GDT_CODE_DESCRIPTOR      \
  DECLARE_GDT_ENTRY(             \
      0, 0, GDT_FLAG_64BIT_MODE, \
      GDT_ACCESS_PRESENT | GDT_ACCESS_PRIVILEGE_RING0 | GDT_ACCESS_EXECUTABLE)

#define GDT_DATA_DESCRIPTOR                    \
  DECLARE_GDT_ENTRY(0, 0, GDT_FLAG_64BIT_MODE, \
                    GDT_ACCESS_PRESENT | GDT_ACCESS_PRIVILEGE_RING0)

#define KERNEL_GDT_ENTRY 2
