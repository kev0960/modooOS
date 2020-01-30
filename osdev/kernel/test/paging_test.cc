#include "../kernel/paging.h"
#include "../kernel/cpp_macro.h"
#include "kernel_test.h"
#include "mock_reg_access.h"

namespace Kernel {
namespace kernel_test {

// We need to provide visibility so this cannot be in the anonymous namespace.
struct MockCPUAccessProvider
    : public MockCPUAccessProviderDefault<MockCPUAccessProvider> {
  void MockSetCR3(uint64_t cr3) override { cr3_ = cr3; }

  uint64_t cr3_;
};

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

bool IsPresent(uint64_t entry) { return entry & 1; }

[[maybe_unused]] uint64_t GetPageTableEntry(uint64_t* pml4_base_addr, uint64_t addr) {
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

[[maybe_unused]] bool IsPagePresent(uint64_t* pml4_base_addr, uint64_t addr) {
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
  KernelPageTable kernel_page_table;
  const size_t start_addr = 0xFFFFFFFF80000000LL;
  const size_t size = 1 << 30;

  kernel_page_table.Alloc4KPagesForKernel<MockCPUAccessProvider>(start_addr,
                                                                 size);
  uint64_t* pml4e_base_addr = PhysToKernel<uint64_t*>(
      GetBaseAddress(MockCPUAccessProvider::GetMock().cr3_));
  for (size_t addr = start_addr; addr < start_addr + size; addr += (1 << 5)) {
    EXPECT_TRUE(IsPresent(GetPageTableEntry(pml4e_base_addr, addr)));
  }
  EXPECT_FALSE(IsPagePresent(pml4e_base_addr, start_addr - 1));
  EXPECT_FALSE(IsPagePresent(pml4e_base_addr, start_addr + size));
}

TEST(PagingTest, Init3MBPaging) {
  KernelPageTable kernel_page_table;
  const size_t start_addr = 0xFFFFFFFF80000000LL;
  const size_t size = (1 << 20) * 3;

  kernel_page_table.Alloc4KPagesForKernel<MockCPUAccessProvider>(start_addr,
                                                                 size);
  uint64_t* pml4e_base_addr = PhysToKernel<uint64_t*>(
      GetBaseAddress(MockCPUAccessProvider::GetMock().cr3_));
  for (size_t addr = start_addr; addr < start_addr + size; addr += 1) {
    EXPECT_TRUE(IsPresent(GetPageTableEntry(pml4e_base_addr, addr)));
  }
  EXPECT_FALSE(IsPagePresent(pml4e_base_addr, start_addr - 1));
  EXPECT_FALSE(IsPagePresent(pml4e_base_addr, start_addr + size));
}

TEST(PagingTest, AllocBunchOf2MBPagingWeird4KBoundary) {
  KernelPageTable kernel_page_table;
  size_t start_addr = 0xFFFFFFFF80000000LL + (1 << 12) * 7;
  size_t size = (1 << 20) * 2;

  kernel_page_table.Alloc4KPagesForKernel<MockCPUAccessProvider>(start_addr,
                                                                 size);
  uint64_t* pml4e_base_addr = PhysToKernel<uint64_t*>(
      GetBaseAddress(MockCPUAccessProvider::GetMock().cr3_));
  for (size_t addr = start_addr; addr < start_addr + size; addr += 2) {
    EXPECT_TRUE(IsPresent(GetPageTableEntry(pml4e_base_addr, addr)));
  }
  EXPECT_FALSE(IsPagePresent(pml4e_base_addr, start_addr - 1));
  EXPECT_FALSE(IsPagePresent(pml4e_base_addr, start_addr + size));

  start_addr = 0xFFFFFFFF80000000LL + (1 << 20) * 6 + (1 << 12) * 123;
  size = (1 << 20) * 2;
  kernel_page_table.Alloc4KPagesForKernel<MockCPUAccessProvider>(start_addr,
                                                                 size);
  for (size_t addr = start_addr; addr < start_addr + size; addr += 2) {
    EXPECT_TRUE(IsPresent(GetPageTableEntry(pml4e_base_addr, addr)));
  }
  EXPECT_FALSE(IsPagePresent(pml4e_base_addr, start_addr - 1));
  EXPECT_FALSE(IsPagePresent(pml4e_base_addr, start_addr + size));

  start_addr = 0xFFFFFFFF80000000LL + (1 << 25) * 6 + (1 << 15) * 21;
  size = (1 << 20) * 2;
  kernel_page_table.Alloc4KPagesForKernel<MockCPUAccessProvider>(start_addr,
                                                                 size);
  for (size_t addr = start_addr; addr < start_addr + size; addr += 2) {
    EXPECT_TRUE(IsPresent(GetPageTableEntry(pml4e_base_addr, addr)));
  }
  EXPECT_FALSE(IsPagePresent(pml4e_base_addr, start_addr - 1));
  EXPECT_FALSE(IsPagePresent(pml4e_base_addr, start_addr + size));
}

}  // namespace
}  // namespace kernel_test
}  // namespace Kernel
