#define GDT_TABLE_SIZE 0x800
#define GDT_ENTRY_SIZE 8

#define GDT_FLAG_FOUR_KILOBYTE_GRANULARITY (1 << 3)
#define GDT_FLAG_32BIT_PROTECTED_MODE (1 << 2)
#define GDT_FLAG_16BIT_REAL_MODE (0 << 2)
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

#define GDT_CODE_DESCRIPTOR16               \
  DECLARE_GDT_ENTRY(                        \
      0, 0xFFFFF, GDT_FLAG_16BIT_REAL_MODE, \
      GDT_ACCESS_PRESENT | GDT_ACCESS_PRIVILEGE_RING0 | GDT_ACCESS_EXECUTABLE)

#define GDT_DATA_DESCRIPTOR16                             \
  DECLARE_GDT_ENTRY(0, 0xFFFFF, GDT_FLAG_16BIT_REAL_MODE, \
                    GDT_ACCESS_PRESENT | GDT_ACCESS_PRIVILEGE_RING0)

#define GDT_CODE_DESCRIPTOR32                  \
  DECLARE_GDT_ENTRY(                           \
      0, 0xFFFFFFFF, GDT_FLAG_16BIT_REAL_MODE, \
      GDT_ACCESS_PRESENT | GDT_ACCESS_PRIVILEGE_RING0 | GDT_ACCESS_EXECUTABLE)

#define GDT_DATA_DESCRIPTOR32                                \
  DECLARE_GDT_ENTRY(0, 0xFFFFFFFF, GDT_FLAG_16BIT_REAL_MODE, \
                    GDT_ACCESS_PRESENT | GDT_ACCESS_PRIVILEGE_RING0)

