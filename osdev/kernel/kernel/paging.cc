#include "paging.h"
#include "../std/algorithm.h"
#include "../std/printf.h"
#include "kernel_util.h"
#include "kmalloc.h"

namespace Kernel {
namespace {

constexpr uint64_t kKernelVirtualOffset = 0xFFFFFFFF80000000;
constexpr uint64_t FourKB = 0x1000;

constexpr uint64_t kPML4AddressSizePerEntry = (1LL << 39);
constexpr size_t kPML4EntryNum = 512;

constexpr uint64_t kPDPTableAddressSizePerEntry = (1LL << 30);
constexpr size_t kPDPTEntryNum = 512;

constexpr uint64_t kPDTableAddressSizePerEntry = (1LL << 21);
constexpr size_t kPDTableEntryNum = 512;

constexpr uint64_t kPageTableAddressSizePerEntry = (1LL << 12);
constexpr size_t kPageTableEntryNum = 512;

void SetPresent(uint64_t* entry) { (*entry) |= 1; }
bool IsPresent(uint64_t entry) { return entry & 1; }

void SetReadWrite(uint64_t* entry) { (*entry) |= 0x2; }

void SetSupervisor(uint64_t* entry) { (*entry) |= 0x4; }

void SetBaseAddress(uint64_t base_addr, uint64_t* entry) {
  ASSERT(base_addr % FourKB == 0);
  (*entry) |= base_addr;
}

uint64_t* GetBaseAddress(uint64_t entry) {
  return reinterpret_cast<uint64_t*>(entry & (0x000FFFFFFFFFF000LL));
}

template <typename T, typename U>
T KernelToPhys(U* kernel_virtual_addr) {
  return reinterpret_cast<T>(reinterpret_cast<uint64_t>(kernel_virtual_addr) -
                             kKernelVirtualOffset);
}

template <typename T, typename U>
T PhysToKernel(U* physical_addr) {
  return reinterpret_cast<T>(reinterpret_cast<uint64_t>(physical_addr) +
                             kKernelVirtualOffset);
}

size_t GetPML4Offset(uint64_t addr) {
  addr = addr >> 39;
  return addr % 512;
}

uint64_t GetPML4StartAddr(uint64_t addr) { return (addr >> 39) << 39; }

size_t GetPDPOffset(uint64_t addr) {
  addr = addr >> 30;
  return addr % 512;
}

uint64_t GetPDPStartAddr(uint64_t addr) { return (addr >> 30) << 30; }

size_t GetPDOffset(uint64_t addr) {
  addr = addr >> 21;
  return addr % 512;
}

uint64_t GetPDStartAddr(uint64_t addr) { return (addr >> 21) << 21; }

size_t GetPTOffset(uint64_t addr) {
  addr = addr >> 12;
  return addr % 512;
}

uint64_t GetPTStartAddr(uint64_t addr) { return (addr >> 12) << 12; }

// For 4KB Paging, all the page table entries share the similar structure. We
// don't have to define specific functions for each type of tables.
void SetEntry(uint64_t page_dir_pointer_addr, bool present, bool rw, bool super,
              uint64_t* entry) {
  // Clear the entry to 0.
  *entry = 0;

  if (present) {
    SetPresent(entry);
  }

  if (rw) {
    SetReadWrite(entry);
  }

  if (super) {
    SetSupervisor(entry);
  }

  // Note page_dir_pointer_addr is assumed to be 4KB aligned.
  ASSERT(page_dir_pointer_addr % 0x1000 == 0);
  SetBaseAddress(page_dir_pointer_addr, entry);
}

// Always returns physical address. The actual table lives in the kernel memory.
uint64_t* CreateNewTable() {
  uint64_t* table_base_addr =
      KernelToPhys<uint64_t*>(kaligned_alloc(FourKB, sizeof(uint64_t) * 512));

  // Zero initialize.
  for (size_t i = 0; i < kPDPTEntryNum; i++) {
    SetEntry(0, /*present=*/false, false, false, &table_base_addr[i]);
  }

  return table_base_addr;
}

}  // namespace

void KernelPageTable::Init4KBPaging(uint64_t kernel_vm_start_addr,
                                    size_t bytes) {
  // First initialize the Page-Map Level-4 Table (the first table).
  pml4e_base_addr_ = KernelToPhys<uint64_t*>(
      kaligned_alloc(FourKB, sizeof(uint64_t) * kPML4EntryNum));

  // Zero initialize.
  for (size_t i = 0; i < 512; i++) {
    SetEntry(0, /*present=*/false, false, false, &pml4e_base_addr_[i]);
  }

  // Initialize kernel paging.
  SetPML4E(kernel_vm_start_addr, bytes);
}

void KernelPageTable::RegisterKernelPage(uint64_t start_addr, uint64_t size) {
  // size should be multiple of 4kb.
  ASSERT(size % FourKB == 0);
  ASSERT(start_addr % FourKB == 0);
}

void KernelPageTable::SetPML4E(uint64_t start_addr, uint64_t size) {
  size_t offset_start = GetPML4Offset(start_addr);
  size_t offset_end = GetPML4Offset(start_addr + size - 1);
  uint64_t pml4_start_addr = GetPML4StartAddr(start_addr);

  kprintf("start : %d end : %d start : %lx \n", offset_start, offset_end,
          pml4_start_addr);
  for (size_t offset = offset_start; offset <= offset_end; offset++) {
    if (!IsPresent(pml4e_base_addr_[offset])) {
      // We need to create a Page directory pointer table (Level 3).
      uint64_t* pdpt_base_addr = CreateNewTable();
      SetEntry((uint64_t)pdpt_base_addr, /*present=*/true, /*rw=*/true,
               /*super=*/true, &pml4e_base_addr_[offset]);
    }
    uint64_t* pdpt_base_addr =
        PhysToKernel<uint64_t*>(GetBaseAddress(pml4e_base_addr_[offset]));
    int delta = offset - offset_start;
    SetPDPT(max(start_addr, pml4_start_addr + delta * kPML4AddressSizePerEntry),
            min(start_addr + size,
                pml4_start_addr + (delta + 1) * kPML4AddressSizePerEntry),
            pdpt_base_addr);
  }
}

void KernelPageTable::SetPDPT(uint64_t start_addr, uint64_t end_addr,
                              uint64_t* pdpe_base_addr) {
  size_t offset_start = GetPDPOffset(start_addr);
  size_t offset_end = GetPDPOffset(end_addr - 1);
  uint64_t pdpt_start_addr = GetPDPStartAddr(start_addr);

  kprintf("[PDPT] start : %d end : %d base : %lx start : %lx \n", offset_start,
          offset_end, pdpe_base_addr, pdpt_start_addr);
  for (size_t offset = offset_start; offset <= offset_end; offset++) {
    if (!IsPresent(pdpe_base_addr[offset])) {
      uint64_t* pdt_base_addr = CreateNewTable();
      SetEntry((uint64_t)pdt_base_addr, /*present=*/true, /*rw=*/true,
               /*super=*/true, &pdpe_base_addr[offset]);
    }
    uint64_t* pdt_base_addr =
        PhysToKernel<uint64_t*>(GetBaseAddress(pdpe_base_addr[offset]));
    int delta = offset - offset_start;
    SetPDT(
        max(start_addr, pdpt_start_addr + delta * kPDPTableAddressSizePerEntry),
        min(end_addr,
            pdpt_start_addr + (delta + 1) * kPDPTableAddressSizePerEntry),
        pdt_base_addr);
  }
}

void KernelPageTable::SetPDT(uint64_t start_addr, uint64_t end_addr,
                             uint64_t* pdt_base_addr) {
  size_t offset_start = GetPDOffset(start_addr);
  size_t offset_end = GetPDOffset(end_addr - 1);
  uint64_t pdt_start_addr = GetPDStartAddr(start_addr);

  kprintf("[PDT] start : %d end : %d base : %lx start : %lx \n", offset_start,
          offset_end, pdt_base_addr, pdt_start_addr);
  for (size_t offset = offset_start; offset <= offset_end; offset++) {
    if (!IsPresent(pdt_base_addr[offset])) {
      uint64_t* pt_base_addr = CreateNewTable();
      SetEntry((uint64_t)pt_base_addr, /*present=*/true, /*rw=*/true,
               /*super=*/true, &pdt_base_addr[offset]);
    }
    uint64_t* pt_base_addr =
        PhysToKernel<uint64_t*>(GetBaseAddress(pdt_base_addr[offset]));
    int delta = offset - offset_start;
    SetPT(
        max(start_addr, pdt_start_addr + delta * kPDPTableAddressSizePerEntry),
        min(end_addr,
            pdt_start_addr + (delta + 1) * kPDTableAddressSizePerEntry),
        pt_base_addr);
  }
}

void KernelPageTable::SetPT(uint64_t start_addr, uint64_t end_addr,
                            uint64_t* pt_base_addr) {
  size_t offset_start = GetPTOffset(start_addr);
  size_t offset_end = GetPTOffset(end_addr - 1);
  uint64_t pt_start_addr = GetPTStartAddr(start_addr);

  /*
  kprintf("start : %d end : %d base : %lx start : %lx \n", offset_start,
          offset_end, pt_base_addr, pt_start_addr);
          */
  for (size_t offset = offset_start; offset <= offset_end; offset++) {
    int delta = offset - offset_start;
    SetEntry(pt_start_addr + delta * kPageTableAddressSizePerEntry, true, true,
             true, &pt_base_addr[offset]);
  }
}

template <typename GetOffset, typename GetStartAddr,
          typename SetNextLevelPageTable, size_t AddressSpaceSizePerEntry>
void KernelPageTable::SetTableEntry(uint64_t start_addr, uint64_t end_addr,
                                    uint64_t* table_base_addr) {
  size_t offset_start = GetOffset(start_addr);
  size_t offset_end = GetOffset(end_addr - 1);
  uint64_t entry_start_addr = GetStartAddr(start_addr);

  for (size_t offset = offset_start; offset <= offset_end; offset++) {
    if (!IsPresent(table_base_addr[offset])) {
      uint64_t* next_level_page_base_addr = CreateNewTable();
      SetEntry((uint64_t)next_level_page_base_addr, /*present=*/true,
               /*rw=*/true, /*super=*/true, &table_base_addr[offset]);
    }
    uint64_t* next_level_page_base_addr =
        PhysToKernel<uint64_t*>(GetBaseAddress(table_base_addr[offset]));
    int delta = offset - offset_start;
    SetNextLevelPageTable(
        max(start_addr, entry_start_addr + delta * AddressSpaceSizePerEntry),
        min(end_addr,
            entry_start_addr + (delta + 1) * AddressSpaceSizePerEntry),
        next_level_page_base_addr);
  }
}

}  // namespace Kernel
