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

class KernelPageTable {
 public:
  KernelPageTable() : pml4e_base_addr_(nullptr), cr3_(0) {}

  template <typename RegsAccessProvider>
  void Alloc4KPagesForKernel(uint64_t kernel_vm_start_addr, size_t bytes) {
    uint64_t cr3 =
        Alloc4KBZeroPagesAndGetCR3(kernel_vm_start_addr, bytes, true);
    if (cr3 != cr3_) {
      RegsAccessProvider::SetCR3(cr3);
      cr3_ = cr3;
    }
  }
  template <typename RegsAccessProvider>
  void Alloc4KPagesForUser(uint64_t user_vm_start_addr, size_t bytes) {
    ASSERT(user_vm_start_addr < 0xFFFFFFFF'80000000ULL);
    uint64_t cr3 = Alloc4KBZeroPagesAndGetCR3(user_vm_start_addr, bytes,
                                              /*is_kernel=*/false);
    if (cr3 != cr3_) {
      RegsAccessProvider::SetCR3(cr3);
      cr3_ = cr3;
    }
  }

 private:
  uint64_t Alloc4KBZeroPagesAndGetCR3(uint64_t vm_start_addr, size_t bytes,
                                      bool is_kernel);
  void SetPML4E(uint64_t start_addr, uint64_t size, bool is_kernel);
  // [start_addr, end_addr)
  void SetPDPT(uint64_t start_addr, uint64_t end_addr, uint64_t* pdpe_base_addr,
               bool is_kernel);
  void SetPDT(uint64_t start_addr, uint64_t size, uint64_t* pdt_base_addr,
              bool is_kernel);
  void SetPT(uint64_t start_addr, uint64_t size, uint64_t* pt_base_addr,
             bool is_kernel);

  template <typename GetOffset, typename GetStartAddr,
            typename SetNextLevelPageTable, size_t AddressSpaceSizePerEntry>
  void SetTableEntry(uint64_t start_addr, uint64_t end_addr,
                     uint64_t* table_base_addr);

  // Register start_addr ~ start_addr + size address as Kernel Page.
  void RegisterKernelPage(uint64_t start_addr, uint64_t size);
  uint64_t* pml4e_base_addr_;
  uint64_t cr3_;

  // Kenrel pages can be shared accross page tables of the user program.
  std::vector<std::pair</*offset*/ size_t, /*entry*/ uint64_t>>
      shared_kernel_pml4e_entries_;
};

class UserPageAllocator {};
}  // namespace Kernel

#endif
