#ifndef PAGING_H
#define PAGING_H

#include "../std/types.h"

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
  void Init4KBPaging(uint64_t kernel_vm_start_addr, size_t bytes);

 private:
  void SetPML4E(uint64_t start_addr, uint64_t size);
  // [start_addr, end_addr)
  void SetPDPT(uint64_t start_addr, uint64_t end_addr,
               uint64_t* pdpe_base_addr);
  void SetPDT(uint64_t start_addr, uint64_t size, uint64_t* pdt_base_addr);
  void SetPT(uint64_t start_addr, uint64_t size, uint64_t* pt_base_addr);

  template <typename GetOffset, typename GetStartAddr,
            typename SetNextLevelPageTable, size_t AddressSpaceSizePerEntry>
  void SetTableEntry(uint64_t start_addr, uint64_t end_addr,
                     uint64_t* table_base_addr);

  // Register start_addr ~ start_addr + size address as Kernel Page.
  void RegisterKernelPage(uint64_t start_addr, uint64_t size);
  uint64_t* pml4e_base_addr_;
};

class UserPageAllocator {};
}  // namespace Kernel

#endif
