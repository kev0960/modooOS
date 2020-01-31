#include "../kernel/paging.h"
#include "../kernel/cpp_macro.h"
#include "kernel_test.h"

namespace Kernel {
namespace kernel_test {
namespace {

uint64_t* GetBaseAddress(uint64_t entry) {
  return reinterpret_cast<uint64_t*>(entry & (0x000FFFFFFFFFF000LL));
}

template <typename T, typename U>
T KernelToPhys(U* kernel_virtual_addr) {
  return reinterpret_cast<T>(reinterpret_cast<uint64_t>(kernel_virtual_addr) -
                             0xFFFFFFFF80000000);
}

template <typename T, typename U>
T PhysToKernel(U* physical_addr) {
  return reinterpret_cast<T>(reinterpret_cast<uint64_t>(physical_addr) +
                             0xFFFFFFFF80000000);
}

size_t GetPML4Offset(uint64_t addr) {
  addr = addr >> 39;
  return addr % 512;
}

size_t GetPDPOffset(uint64_t addr) {
  addr = addr >> 30;
  return addr % 512;
}

size_t GetPDOffset(uint64_t addr) {
  addr = addr >> 21;
  return addr % 512;
}

size_t GetPTOffset(uint64_t addr) {
  addr = addr >> 12;
  return addr % 512;
}

[[maybe_unused]] size_t GetPhysicalPageOffset(uint64_t addr) {
  return addr % (1 << 12);
}

bool IsPresent(uint64_t entry) {
  return entry & 1;
}
bool IsSupervisor(uint64_t entry) { return entry & 4; }

uint64_t GetPageTableEntry(uint64_t* pml4_base_addr, uint64_t addr) {
  size_t pml4_offset = GetPML4Offset(addr);
  EXPECT_TRUE(IsPresent(pml4_base_addr[pml4_offset]));
  uint64_t* pdp_base_addr = GetBaseAddress(pml4_base_addr[pml4_offset]);

  size_t pdp_offset = GetPDPOffset(addr);
  EXPECT_TRUE(IsPresent(pdp_base_addr[pdp_offset]));
  uint64_t* pd_base_addr = GetBaseAddress(pdp_base_addr[pdp_offset]);

  size_t pd_offset = GetPDOffset(addr);
  EXPECT_TRUE(IsPresent(pd_base_addr[pd_offset]));
  uint64_t* pt_base_addr = GetBaseAddress(pd_base_addr[pd_offset]);

  size_t pt_offset = GetPTOffset(addr);
  return pt_base_addr[pt_offset];
}

bool IsPagePresent(uint64_t* pml4_base_addr, uint64_t addr) {
  size_t pml4_offset = GetPML4Offset(addr);
  if (!IsPresent(pml4_base_addr[pml4_offset])) {
    return false;
  }
  uint64_t* pdp_base_addr = GetBaseAddress(pml4_base_addr[pml4_offset]);

  size_t pdp_offset = GetPDPOffset(addr);
  if (!IsPresent(pdp_base_addr[pml4_offset])) {
    return false;
  }
  uint64_t* pd_base_addr = GetBaseAddress(pdp_base_addr[pdp_offset]);

  size_t pd_offset = GetPDOffset(addr);
  if (!IsPresent(pd_base_addr[pml4_offset])) {
    return false;
  }
  uint64_t* pt_base_addr = GetBaseAddress(pd_base_addr[pd_offset]);

  size_t pt_offset = GetPTOffset(addr);
  return IsPresent(pt_base_addr[pt_offset]);
}

TEST(PagingTest, CheckTestFunctions) {
  // 1111111111111111111100010010001101000101011001111000100100001010
  // 0123456789012345678901234567890123456789012345678901234567890123
  //                 |__ PML4 |        |        |        |
  //                 111100010|__ PDP  |        |        |
  //                          010001101|__ PD   |        |
  //                                   000101011|__PT    |
  //                                            001111000|__Physical Page
  uint64_t addr = 0xFFFFF123'4567890A;
  EXPECT_EQ(GetPML4Offset(addr), 0b111100010ULL);
  EXPECT_EQ(GetPDPOffset(addr), 0b010001101ULL);
  EXPECT_EQ(GetPDOffset(addr), 0b000101011ULL);
  EXPECT_EQ(GetPTOffset(addr), 0b001111000ULL);
}

TEST(PagingTest, Init1GBPaging) {
  PageTable kernel_page_table;
  const size_t start_addr = 0xFFFFFFFF80000000LL;
  const size_t size = 1 << 30;

  uint64_t* pml4e_base_addr = kernel_page_table.CreateEmptyPageTable();
  kernel_page_table.AllocateTable(pml4e_base_addr, start_addr, size, true,
                                  start_addr);

  for (size_t addr = start_addr; addr < start_addr + size; addr += (1 << 5)) {
    EXPECT_TRUE(IsPresent(GetPageTableEntry(pml4e_base_addr, addr)));
  }
  EXPECT_FALSE(IsPagePresent(pml4e_base_addr, start_addr - 1));
  EXPECT_FALSE(IsPagePresent(pml4e_base_addr, start_addr + size));
}

TEST(PagingTest, Init3MBPaging) {
  PageTable kernel_page_table;
  const size_t start_addr = 0xFFFFFFFF80000000LL;
  const size_t size = (1 << 20) * 3;

  uint64_t* pml4e_base_addr = kernel_page_table.CreateEmptyPageTable();
  kernel_page_table.AllocateTable(pml4e_base_addr, start_addr, size, true,
                                  start_addr);

  for (size_t addr = start_addr; addr < start_addr + size; addr += 1) {
    EXPECT_TRUE(IsPresent(GetPageTableEntry(pml4e_base_addr, addr)));
  }
  EXPECT_FALSE(IsPagePresent(pml4e_base_addr, start_addr - 1));
  EXPECT_FALSE(IsPagePresent(pml4e_base_addr, start_addr + size));
}

TEST(PagingTest, AllocBunchOf2MBPagingWeird4KBoundary) {
  PageTable kernel_page_table;
  size_t start_addr = 0xFFFFFFFF80000000LL + (1 << 12) * 7;
  size_t size = (1 << 20) * 2;

  uint64_t* pml4e_base_addr = kernel_page_table.CreateEmptyPageTable();
  kernel_page_table.AllocateTable(pml4e_base_addr, start_addr, size, true,
                                  start_addr);

  for (size_t addr = start_addr; addr < start_addr + size; addr += 2) {
    EXPECT_TRUE(IsPresent(GetPageTableEntry(pml4e_base_addr, addr)));
  }
  EXPECT_FALSE(IsPagePresent(pml4e_base_addr, start_addr - 1));
  EXPECT_FALSE(IsPagePresent(pml4e_base_addr, start_addr + size));

  start_addr = 0xFFFFFFFF80000000LL + (1 << 20) * 6 + (1 << 12) * 123;
  size = (1 << 20) * 2;
  kernel_page_table.AllocateTable(pml4e_base_addr, start_addr, size, true,
                                  start_addr);

  for (size_t addr = start_addr; addr < start_addr + size; addr += 2) {
    EXPECT_TRUE(IsPresent(GetPageTableEntry(pml4e_base_addr, addr)));
  }
  EXPECT_FALSE(IsPagePresent(pml4e_base_addr, start_addr - 1));
  EXPECT_FALSE(IsPagePresent(pml4e_base_addr, start_addr + size));

  start_addr = 0xFFFFFFFF80000000LL + (1 << 25) * 6 + (1 << 15) * 21;
  size = (1 << 20) * 2;
  kernel_page_table.AllocateTable(pml4e_base_addr, start_addr, size, true,
                                  start_addr);

  for (size_t addr = start_addr; addr < start_addr + size; addr += 2) {
    EXPECT_TRUE(IsPresent(GetPageTableEntry(pml4e_base_addr, addr)));
  }
  EXPECT_FALSE(IsPagePresent(pml4e_base_addr, start_addr - 1));
  EXPECT_FALSE(IsPagePresent(pml4e_base_addr, start_addr + size));
}

TEST(PagingTest, AllocUserPage) {
  PageTable kernel_page_table;
  size_t start_addr = 0xFFFFFFFF80000000LL;
  size_t size = (1 << 20);

  // First Alloc kernel page.
  // /*
  uint64_t* kernel_pml4e_base_addr = kernel_page_table.CreateEmptyPageTable();
  kernel_page_table.AllocateTable(kernel_pml4e_base_addr, start_addr, size,
                                  true, start_addr);

  for (size_t addr = start_addr; addr < start_addr + size; addr += 2) {
    EXPECT_TRUE(IsPresent(GetPageTableEntry(kernel_pml4e_base_addr, addr)));
    EXPECT_TRUE(IsSupervisor(GetPageTableEntry(kernel_pml4e_base_addr, addr)));
  }
  EXPECT_FALSE(IsPagePresent(kernel_pml4e_base_addr, start_addr - 1));
  EXPECT_FALSE(IsPagePresent(kernel_pml4e_base_addr, start_addr + size));

  // Now alloc user page.
  uint64_t* user_pml4e_base_addr = kernel_page_table.CreateEmptyPageTable();
  size_t user_start_addr = 0x10000000;
  size_t user_physical_addr = 0x12345000;
  kernel_page_table.AllocateTable(user_pml4e_base_addr, user_start_addr, size,
                                  false, user_physical_addr);

  // Check that user pages are properly allocated.
  for (size_t addr = user_start_addr; addr < user_start_addr + size;
       addr += 2) {
    size_t user_entry = GetPageTableEntry(user_pml4e_base_addr, addr);
    EXPECT_TRUE(IsPresent(user_entry));
    size_t offset = GetPhysicalPageOffset(addr);
    uint64_t* physical_page_table_start = GetBaseAddress(user_entry);

    EXPECT_EQ((uint64_t)(physical_page_table_start) + offset,
              addr + (user_physical_addr - user_start_addr));
    EXPECT_FALSE(IsSupervisor(GetPageTableEntry(user_pml4e_base_addr, addr)));
  }
  EXPECT_FALSE(IsPagePresent(user_pml4e_base_addr, user_start_addr - 1));
  EXPECT_FALSE(IsPagePresent(user_pml4e_base_addr, user_start_addr + size));

  // Check that kernel page also exists.
  for (size_t addr = start_addr; addr < start_addr + size; addr += 2) {
    EXPECT_TRUE(IsPresent(GetPageTableEntry(user_pml4e_base_addr, addr)));
    EXPECT_TRUE(IsSupervisor(GetPageTableEntry(user_pml4e_base_addr, addr)));
  }
  EXPECT_FALSE(IsPagePresent(user_pml4e_base_addr, start_addr - 1));
  EXPECT_FALSE(IsPagePresent(user_pml4e_base_addr, start_addr + size));
}

}  // namespace
}  // namespace kernel_test
}  // namespace Kernel
