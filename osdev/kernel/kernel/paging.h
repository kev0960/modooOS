#ifndef PAGING_H
#define PAGING_H

#include "../std/types.h"
#include "../std/vector.h"
#include "kernel_util.h"

namespace Kernel {
// Physical Memory Layout
//
//  THIS IS THE PHYSICAL MEMORY (Not Virtual)
//
//   ----------------- End of RAM
//   |               |
//   |               |
//   |               |
//   |   User Mem    |
//   |               |
//   |               |
//   |               |
//   |               |
//   -----------------   0x00000000 3FFFFFFF (1GB)
//   |               |
//   |               |
//   |  Kernel Heap  |
//   |               |
//   -----------------   0x00000000 01400000
//   |     Buffer    |
//   -----------------   0x00000000 ????????
//   |               |
//   | Device Driver |   E.g Vga output : 0x00000000 000b8000
//   |               |
//   -----------------   0x00000000 ????????
//   |               |
//   |  Kernel Stack |   8 KB
//   |               |
//   -----------------   0x00000000 ????????
//   |               |
//   |   Boot Data   |
//   |  structures   |
//   |               |
//   -----------------   0x00000000 00000000
//
// Virtual Memory Layout
//
// 0xFFFFFFFF 80000000 ~ 0xFFFFFFFF BFFFFFFF --> Kernel memory
//
// 0x00000000 00000000 ~ --> User memory.
//

struct PageTableInfo {
  uint64_t* pml4e_base_addr;
};

class PageTable {
 public:
  PageTable() = default;

  // Create an empty page table (only PML4E table is created). Returns the base
  // offset of the PML4E Table.
  uint64_t* CreateEmptyPageTable() const;

  void AllocateTable(uint64_t* pml4e_base_addr, uint64_t vm_start_addr,
                     size_t bytes, bool is_kernel,
                     uint64_t physical_addr_start);

 private:
  void SetPML4E(uint64_t start_addr, uint64_t size, uint64_t* pml4e_base_addr,
                bool is_kernel, uint64_t physical_addr_start);

  // [start_addr, end_addr)
  void SetPDPT(uint64_t start_addr, uint64_t end_addr, uint64_t* pdpe_base_addr,
               bool is_kernel, uint64_t physical_addr_start);
  void SetPDT(uint64_t start_addr, uint64_t end_addr, uint64_t* pdt_base_addr,
              bool is_kernel, uint64_t physical_addr_start);
  void SetPT(uint64_t start_addr, uint64_t end_addr, uint64_t* pt_base_addr,
             bool is_kernel, uint64_t physical_addr_start);
  void Set4KBPhysicalPage(uint64_t start_addr, uint64_t end_addr,
                          uint64_t* physical_table_base_addr,
                          uint64_t physical_addr_start);

  // Register start_addr ~ start_addr + size address as Kernel Page.
  void RegisterKernelPage(uint64_t start_addr, uint64_t size);

  // Kenrel pages can be shared accross page tables of the user program.
  std::vector<std::pair</*offset*/ size_t, /*entry*/ uint64_t>>
      shared_kernel_pml4e_entries_;
};

class UserPageAllocator {};
}  // namespace Kernel

#endif
