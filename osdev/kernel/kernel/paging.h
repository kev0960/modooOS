#ifndef PAGING_H
#define PAGING_H

#include "../std/types.h"
#include "../std/vector.h"
#include "interrupt.h"
#include "kernel_util.h"
#include "printf.h"

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
  // NOTE: THIS ALWAYS RETURNS A PHYSICAL ADDRESS. MUST BE CONVERTED TO KERNEL
  // VIRTUAL ADDRESS IF YOU WANT TO DEREFERENCE IT.
  uint64_t* CreateEmptyPageTable() const;

  void AllocateTable(uint64_t* pml4e_base_addr_phys, uint64_t vm_start_addr,
                     size_t bytes, bool is_kernel,
                     uint64_t physical_addr_start);

  // Remove pages from the table.
  void DeallocatePages(uint64_t* pml4e_base_addr_phys, uint64_t vm_start_addr,
                       size_t bytes);

  // This is for low memory (< 1MB). MUST BE DEALLOCATED AFTER USE.
  // USE AT YOUR OWN DISCRETION!
  void CreateIdentityForKernel(uint64_t* pml4e_base_addr_phys,
                               uint64_t phys_start_addr, size_t bytes);

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

  void FreePML4E(uint64_t start_addr, uint64_t size, uint64_t* pml4e_base_addr);

  // Returns true if every entry in this table is freed.
  bool FreePDPT(uint64_t start_addr, uint64_t end_addr,
                uint64_t* pdpe_base_addr);
  bool FreePDT(uint64_t start_addr, uint64_t end_addr, uint64_t* pdt_base_addr);
  bool FreePT(uint64_t start_addr, uint64_t end_addr, uint64_t* pt_base_addr);

  // Register start_addr ~ start_addr + size address as Kernel Page.
  void RegisterKernelPage(uint64_t start_addr, uint64_t size);

  // Kenrel pages can be shared accross page tables of the user program.
  std::vector<std::pair</*offset*/ size_t, /*entry*/ uint64_t>>
      shared_kernel_pml4e_entries_;
};

class PageTableManager {
 public:
  static constexpr uint64_t kKernelMemorySize = (1 << 30);

  static PageTableManager& GetPageTableManager() {
    static PageTableManager page_table_manager;
    return page_table_manager;
  }

  static constexpr uint64_t kKernelVMStart = 0xFFFF'FFFF'8000'0000ULL;

  uint64_t* GetKernelPml4eBaseAddr() { return kernel_pml4e_base_phys_addr_; }
  template <typename CPURegsAccessProvider>
  void SetCR3(uint64_t* pml4e_base_phys_addr) {
    uint64_t base_addr = reinterpret_cast<uint64_t>(pml4e_base_phys_addr);
    ASSERT(base_addr % (1 << 12) == 0);
    CPURegsAccessProvider::SetCR3(base_addr);
  }

  // Returns physical address to the pml4e base address.
  uint64_t* CreateUserPageTable() { return page_table_.CreateEmptyPageTable(); }

  // Allocate 2^order bytes of pages for user_vm_address.
  void AllocatePage(uint64_t* user_pml4e_base_phys_addr_,
                    uint64_t* user_vm_address, size_t order);

  void PageFaultHandler(CPUInterruptHandlerArgs* args,
                        InterruptHandlerSavedRegs* regs);

  // Copy page table of parent process to the user process. Used for fork().
  void CopyUserPageTable(uint64_t* from_pml4_base_addr,
                         uint64_t* to_pml4_base_addr);

  void AllocateKernelPage(uint64_t kernel_vm_addr, uint64_t size,
                          uint64_t physical_addr);

  // This is for low memory (< 1MB). MUST BE DEALLOCATED AFTER USE.
  // USE AT YOUR OWN DISCRETION!
  void CreateIdentityForKernel(uint64_t phys_start_addr, size_t bytes) {
    ASSERT(phys_start_addr <= 0x10000);
    page_table_.CreateIdentityForKernel(kernel_pml4e_base_phys_addr_,
                                        phys_start_addr, bytes);
  }

 private:
  PageTableManager() {
    // Create PML4E Table for the kernel.
    kernel_pml4e_base_phys_addr_ = page_table_.CreateEmptyPageTable();

    // Map kernel VM to physical memory starting 0.
    page_table_.AllocateTable(kernel_pml4e_base_phys_addr_, kKernelVMStart,
                              kKernelMemorySize, /*is_kernel=*/true,
                              /*physical=*/0);
  }

  PageTable page_table_;
  uint64_t* kernel_pml4e_base_phys_addr_;
};

class PageTablePrintUtil {
 public:
  static void PrintUserTable(uint64_t* cr3);

 private:
  // Level 3
  static void PrintPDPE(uint64_t* pdpt_top, uint64_t start_addr);

  // Level 2
  static void PrintPDE(uint64_t* pdt_top, uint64_t start_addr);

  // Level 1
  static void PrintPT(uint64_t* pt_top, uint64_t start_addr);
};

}  // namespace Kernel

extern "C" void PageFaultInterruptHandlerCaller(
    Kernel::CPUInterruptHandlerArgs* args,
    Kernel::InterruptHandlerSavedRegs* regs);

#endif
