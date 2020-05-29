#include "paging.h"

#include "../std/algorithm.h"
#include "../std/printf.h"
#include "../std/utility.h"
#include "./fs/ext2.h"
#include "cpu.h"
#include "cpu_context.h"
#include "descriptor_table.h"
#include "frame_allocator.h"
#include "kernel_context.h"
#include "kernel_util.h"
#include "kmalloc.h"
#include "kthread.h"
#include "process.h"
#include "qemu_log.h"
#include "vga_output.h"

namespace Kernel {
namespace {

// static bool enable_log = false;

constexpr uint64_t kKernelVirtualOffset = 0xFFFFFFFF'80000000LL;
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
void SetFree(uint64_t* entry) { (*entry) &= (0xFFFFFFFF'FFFFFFFELL); }

bool IsPresent(uint64_t entry) { return entry & 1; }
bool IsUserAccessible(uint64_t entry) { return entry & 0x4; }

void SetReadWrite(uint64_t* entry) { (*entry) |= 0x2; }

void SetUserAccessible(uint64_t* entry) { (*entry) |= 0x4; }

void SetBaseAddress(uint64_t base_addr, uint64_t* entry) {
  ASSERT(base_addr % FourKB == 0);
  (*entry) |= base_addr;
}

uint64_t* GetBaseAddress(uint64_t entry) {
  return reinterpret_cast<uint64_t*>(entry & (0x000FFFFF'FFFFF000LL));
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

// For 4KB Paging, all the page table entries share the similar structure. We
// don't have to define specific functions for each type of tables.
void SetEntry(uint64_t page_dir_pointer_addr, bool present, bool rw,
              bool is_kernel, uint64_t* entry) {
  // Clear the entry to 0.
  *entry = 0;

  if (present) {
    SetPresent(entry);
  }

  if (rw) {
    SetReadWrite(entry);
  }

  // U/S bit. If this bit is set to 1, both supervised and user access is
  // allowed. If cleared to 0, then only restricted to supervisor level
  // (CPL=0,1,2)
  if (!is_kernel) {
    SetUserAccessible(entry);
  }

  // Note page_dir_pointer_addr is assumed to be 4KB aligned.
  ASSERT(page_dir_pointer_addr % 0x1000 == 0);
  SetBaseAddress(page_dir_pointer_addr, entry);
}

// Always returns physical address. The actual table lives in the kernel memory.
uint64_t* CreateNewTable() {
  uint64_t* table_base_addr = reinterpret_cast<uint64_t*>(
      kaligned_alloc(FourKB, sizeof(uint64_t) * 512));

  ASSERT(table_base_addr != nullptr);

  // Zero initialize.
  for (size_t i = 0; i < kPDPTEntryNum; i++) {
    SetEntry(0, /*present=*/false, false, false, &table_base_addr[i]);
  }

  return table_base_addr;
}

// Get the 4KB boundary address.
constexpr uint64_t Get4KBBoundary(uint64_t addr) {
  return addr & (0xFFFFFFFF'FFFFF000LL);
}

template <typename T, typename U>
void CopyCPUInteruptHandlerArgs(T* to, U* from) {
  to->rip = from->rip;
  to->rsp = from->rsp;
  to->rflags = from->rflags;
  to->ss = from->ss;
  to->cs = from->cs;
}

}  // namespace

uint64_t* PageTable::CreateEmptyPageTable() const {
  uint64_t* pml4e_base_addr = reinterpret_cast<uint64_t*>(
      kaligned_alloc(FourKB, sizeof(uint64_t) * kPML4EntryNum));
  for (size_t i = 0; i < 512; i++) {
    SetEntry(0, /*present=*/false, false, false, &pml4e_base_addr[i]);
  }

  // Now assign pre-defined kernel pages.
  for (const auto& item : shared_kernel_pml4e_entries_) {
    pml4e_base_addr[item.first] = item.second;
  }

  return KernelToPhys<uint64_t*>(pml4e_base_addr);
}

void PageTable::AllocateTable(uint64_t* pml4e_base_addr_phys,
                              uint64_t vm_start_addr, size_t bytes,
                              bool is_kernel, uint64_t physical_addr_start) {
  if (is_kernel) {
    ASSERT(vm_start_addr >= 0xFFFFFFFF80000000LL);
  } else {
    ASSERT(vm_start_addr < 0xFFFFFFFF80000000LL);
  }

  // Allocate pages.
  SetPML4E(vm_start_addr, bytes, PhysToKernel<uint64_t*>(pml4e_base_addr_phys),
           is_kernel, physical_addr_start);
}

void PageTable::CreateIdentityForKernel(uint64_t* pml4e_base_addr_phys,
                                        uint64_t phys_start_addr,
                                        size_t bytes) {
  SetPML4E(phys_start_addr, bytes,
           PhysToKernel<uint64_t*>(pml4e_base_addr_phys), /*is_kernel=*/true,
           phys_start_addr);
}

void PageTable::SetPML4E(uint64_t start_addr, uint64_t size,
                         uint64_t* pml4e_base_addr, bool is_kernel,
                         uint64_t physical_addr_start) {
  size_t offset_start = GetPML4Offset(start_addr);
  size_t offset_end = GetPML4Offset(start_addr + size - 1);
  uint64_t pml4_start_addr = GetPML4StartAddr(start_addr);

  ASSERT((uint64_t)pml4e_base_addr >= kKernelVirtualOffset);

  for (size_t offset = offset_start; offset <= offset_end; offset++) {
    if (!IsPresent(pml4e_base_addr[offset])) {
      // We need to create a Page directory pointer table (Level 3).
      uint64_t* pdpt_base_addr = CreateNewTable();
      SetEntry(KernelToPhys<uint64_t>(pdpt_base_addr), /*present=*/true,
               /*rw=*/true, /*super=*/is_kernel, &pml4e_base_addr[offset]);

      // If shared kernel's pml4e entry has not been added.
      if (is_kernel &&
          std::find_if(shared_kernel_pml4e_entries_.begin(),
                       shared_kernel_pml4e_entries_.end(),
                       [offset](const auto& item) {
                         return item.first == offset;
                       }) == shared_kernel_pml4e_entries_.end() &&
          /* Only add when it points to real kernel virtual memory.
           * (Since we identically map lower address due to apic & acpi) */
          start_addr >= 0xFFFFFFFF80000000LL) {
        shared_kernel_pml4e_entries_.push_back(
            std::make_pair(offset, pml4e_base_addr[offset]));
      }
    }

    // If setting user mode page, then thise page should be marked as User.
    if (!is_kernel) {
      SetUserAccessible(&pml4e_base_addr[offset]);
    }

    uint64_t* pdpt_base_addr =
        PhysToKernel<uint64_t*>(GetBaseAddress(pml4e_base_addr[offset]));
    int delta = offset - offset_start;

    uint64_t pdpt_start_addr =
        max(start_addr, pml4_start_addr + delta * kPML4AddressSizePerEntry);
    uint64_t pdpt_end_addr = start_addr + size;

    // If pml4 end boundary is not overflown, then compare properly.
    if (pml4_start_addr + (delta + 1) * kPML4AddressSizePerEntry != 0) {
      pdpt_end_addr =
          min(start_addr + size,
              pml4_start_addr + (delta + 1) * kPML4AddressSizePerEntry);
    }

    uint64_t physical_addr_offset = pdpt_start_addr - start_addr;
    SetPDPT(pdpt_start_addr, pdpt_end_addr, pdpt_base_addr, is_kernel,
            physical_addr_start + physical_addr_offset);
  }
}

void PageTable::SetPDPT(uint64_t start_addr, uint64_t end_addr,
                        uint64_t* pdpe_base_addr, bool is_kernel,
                        uint64_t physical_addr_start) {
  size_t offset_start = GetPDPOffset(start_addr);
  size_t offset_end = GetPDPOffset(end_addr - 1);
  uint64_t pdpt_start_addr = GetPDPStartAddr(start_addr);

  ASSERT((uint64_t)pdpe_base_addr >= kKernelVirtualOffset);

  for (size_t offset = offset_start; offset <= offset_end; offset++) {
    if (!IsPresent(pdpe_base_addr[offset])) {
      uint64_t* pdt_base_addr = CreateNewTable();
      SetEntry(KernelToPhys<uint64_t>(pdt_base_addr), /*present=*/true,
               /*rw=*/true, /*super=*/is_kernel, &pdpe_base_addr[offset]);
    }

    // If setting user mode page, then thise page should be marked as User.
    if (!is_kernel) {
      SetUserAccessible(&pdpe_base_addr[offset]);
    }

    uint64_t* pdt_base_addr =
        PhysToKernel<uint64_t*>(GetBaseAddress(pdpe_base_addr[offset]));
    int delta = offset - offset_start;
    uint64_t pdt_start_addr =
        max(start_addr, pdpt_start_addr + delta * kPDPTableAddressSizePerEntry);
    uint64_t physical_addr_offset = pdt_start_addr - start_addr;

    // Set the next level page table.
    SetPDT(pdt_start_addr,
           min(end_addr,
               pdpt_start_addr + (delta + 1) * kPDPTableAddressSizePerEntry),
           pdt_base_addr, is_kernel,
           physical_addr_start + physical_addr_offset);
  }
}

void PageTable::SetPDT(uint64_t start_addr, uint64_t end_addr,
                       uint64_t* pdt_base_addr, bool is_kernel,
                       uint64_t physical_addr_start) {
  size_t offset_start = GetPDOffset(start_addr);
  size_t offset_end = GetPDOffset(end_addr - 1);
  uint64_t pdt_start_addr = GetPDStartAddr(start_addr);

  ASSERT((uint64_t)pdt_base_addr >= kKernelVirtualOffset);

  for (size_t offset = offset_start; offset <= offset_end; offset++) {
    if (!IsPresent(pdt_base_addr[offset])) {
      uint64_t* pt_base_addr = CreateNewTable();
      SetEntry(KernelToPhys<uint64_t>(pt_base_addr), /*present=*/true,
               /*rw=*/true, /*super=*/is_kernel, &pdt_base_addr[offset]);
    }

    // If setting user mode page, then thise page should be marked as User.
    if (!is_kernel) {
      SetUserAccessible(&pdt_base_addr[offset]);
    }

    uint64_t* pt_base_addr =
        PhysToKernel<uint64_t*>(GetBaseAddress(pdt_base_addr[offset]));
    int delta = offset - offset_start;
    uint64_t pt_start_addr =
        max(start_addr, pdt_start_addr + delta * kPDTableAddressSizePerEntry);
    uint64_t physical_addr_offset = pt_start_addr - start_addr;

    // Set the next level page table.
    SetPT(pt_start_addr,
          min(end_addr,
              pdt_start_addr + (delta + 1) * kPDTableAddressSizePerEntry),
          pt_base_addr, is_kernel, physical_addr_start + physical_addr_offset);
  }
}

void PageTable::SetPT(uint64_t start_addr, uint64_t end_addr,
                      uint64_t* pt_base_addr, bool is_kernel,
                      uint64_t physical_addr_start) {
  size_t offset_start = GetPTOffset(start_addr);
  size_t offset_end = GetPTOffset(end_addr - 1);

  ASSERT((uint64_t)pt_base_addr >= kKernelVirtualOffset);

  for (size_t offset = offset_start; offset <= offset_end; offset++) {
    int delta = offset - offset_start;
    SetEntry(physical_addr_start + delta * kPageTableAddressSizePerEntry,
             /*present=*/true, /*rw=*/true, /*super=*/is_kernel,
             &pt_base_addr[offset]);
  }
}

void PageTable::FreePML4E(uint64_t start_addr, uint64_t size,
                          uint64_t* pml4e_base_addr) {
  size_t offset_start = GetPML4Offset(start_addr);
  size_t offset_end = GetPML4Offset(start_addr + size - 1);
  uint64_t pml4_start_addr = GetPML4StartAddr(start_addr);

  ASSERT((uint64_t)pml4e_base_addr >= kKernelVirtualOffset);

  for (size_t offset = offset_start; offset <= offset_end; offset++) {
    if (IsPresent(pml4e_base_addr[offset])) {
      uint64_t* pdpt_base_addr =
          PhysToKernel<uint64_t*>(GetBaseAddress(pml4e_base_addr[offset]));
      int delta = offset - offset_start;

      uint64_t pdpt_start_addr =
          max(start_addr, pml4_start_addr + delta * kPML4AddressSizePerEntry);
      uint64_t pdpt_end_addr = start_addr + size;

      // If pml4 end boundary is not overflown, then compare properly.
      if (pml4_start_addr + (delta + 1) * kPML4AddressSizePerEntry != 0) {
        pdpt_end_addr =
            min(start_addr + size,
                pml4_start_addr + (delta + 1) * kPML4AddressSizePerEntry);
      }

      if (FreePDPT(pdpt_start_addr, pdpt_end_addr, pdpt_base_addr)) {
        SetFree(&pml4e_base_addr[offset]);
      }
    }
  }
}

bool PageTable::FreePDPT(uint64_t start_addr, uint64_t end_addr,
                         uint64_t* pdpe_base_addr) {
  size_t offset_start = GetPDPOffset(start_addr);
  size_t offset_end = GetPDPOffset(end_addr - 1);
  uint64_t pdpt_start_addr = GetPDPStartAddr(start_addr);

  ASSERT((uint64_t)pdpe_base_addr >= kKernelVirtualOffset);

  for (size_t offset = offset_start; offset <= offset_end; offset++) {
    if (IsPresent(pdpe_base_addr[offset])) {
      uint64_t* pdt_base_addr =
          PhysToKernel<uint64_t*>(GetBaseAddress(pdpe_base_addr[offset]));
      int delta = offset - offset_start;
      uint64_t pdt_start_addr = max(
          start_addr, pdpt_start_addr + delta * kPDPTableAddressSizePerEntry);

      // Set the next level page table.
      if (FreePDT(pdt_start_addr,
                  min(end_addr, pdpt_start_addr +
                                    (delta + 1) * kPDPTableAddressSizePerEntry),
                  pdt_base_addr)) {
        SetFree(&pdpe_base_addr[offset]);
      }
    }
  }

  // If everything is free in this table, then we can just free corresponding
  // pml4e entry.
  for (size_t i = 0; i < kPDPTEntryNum; i++) {
    if (IsPresent(pdpe_base_addr[i])) {
      return false;
    }
  }

  return true;
}

bool PageTable::FreePDT(uint64_t start_addr, uint64_t end_addr,
                        uint64_t* pdt_base_addr) {
  size_t offset_start = GetPDOffset(start_addr);
  size_t offset_end = GetPDOffset(end_addr - 1);
  uint64_t pdt_start_addr = GetPDStartAddr(start_addr);

  ASSERT((uint64_t)pdt_base_addr >= kKernelVirtualOffset);

  for (size_t offset = offset_start; offset <= offset_end; offset++) {
    if (IsPresent(pdt_base_addr[offset])) {
      uint64_t* pt_base_addr =
          PhysToKernel<uint64_t*>(GetBaseAddress(pdt_base_addr[offset]));
      int delta = offset - offset_start;
      uint64_t pt_start_addr =
          max(start_addr, pdt_start_addr + delta * kPDTableAddressSizePerEntry);

      // Set the next level page table.
      if (FreePT(pt_start_addr,
                 min(end_addr, pdt_start_addr +
                                   (delta + 1) * kPDTableAddressSizePerEntry),
                 pt_base_addr)) {
        SetFree(&pdt_base_addr[offset]);
      }
    }
  }

  // If everything is free in this table, then we can just free corresponding
  // pdpt entry.
  for (size_t i = 0; i < kPDTableEntryNum; i++) {
    if (IsPresent(pdt_base_addr[i])) {
      return false;
    }
  }

  return true;
}

bool PageTable::FreePT(uint64_t start_addr, uint64_t end_addr,
                       uint64_t* pt_base_addr) {
  size_t offset_start = GetPTOffset(start_addr);
  size_t offset_end = GetPTOffset(end_addr - 1);

  ASSERT((uint64_t)pt_base_addr >= kKernelVirtualOffset);

  for (size_t offset = offset_start; offset <= offset_end; offset++) {
    SetFree(&pt_base_addr[offset]);

    // Free the physical frame.
    uint64_t* frame_phys_addr = GetBaseAddress(pt_base_addr[offset]);
    UserFrameAllocator::GetPhysicalFrameAllocator().FreeFrame(frame_phys_addr);
  }

  for (size_t i = 0; i < kPageTableEntryNum; i++) {
    if (IsPresent(pt_base_addr[i])) {
      return false;
    }
  }
  return true;
}

void PageTableManager::AllocatePage(uint64_t* user_pml4e_base_phys_addr_,
                                    uint64_t* user_vm_address, size_t order) {
  // Make sure that the requested vm address is actually in user space.
  ASSERT((uint64_t)user_vm_address < kKernelVMStart);

  // Get physical frame.
  uint64_t physical_frame = reinterpret_cast<uint64_t>(
      UserFrameAllocator::GetPhysicalFrameAllocator().AllocateFrame(order));
  QemuSerialLog::Logf("Allocate page : %lx (vm: %lx) For [CPU Id :%d] %lx \n",
                      physical_frame, user_vm_address,
                      CPUContextManager::GetCurrentCPUId(),
                      user_pml4e_base_phys_addr_);
  // kprintf("physical frame : %lx \n", physical_frame);
  // Assign physical frames to the page table.
  page_table_.AllocateTable(
      user_pml4e_base_phys_addr_, reinterpret_cast<uint64_t>(user_vm_address),
      (1 << order) * FourKB, /*is_kernel=*/false, physical_frame);
}

void PageTableManager::AllocateKernelPage(uint64_t kernel_vm_addr,
                                          uint64_t size,
                                          uint64_t physical_addr) {
  page_table_.AllocateTable(kernel_pml4e_base_phys_addr_, kernel_vm_addr, size,
                            /*is_kernel=*/true,
                            /*physical=*/physical_addr);
}

void PageTableManager::PageFaultHandler(CPUInterruptHandlerArgs* args,
                                        InterruptHandlerSavedRegs* regs) {
  uint64_t fault_addr = CPURegsAccessProvider::ReadCR2();

  // We first need to check whether the fault address is valid.
  KernelThread* current_thread = KernelThread::CurrentThread();
  Process* process = static_cast<Process*>(current_thread);

  QemuSerialLog::Logf(
      "PF[%lx] at Thread[%d] cpu : [%d] RIP:[%lx] RSP [%lx] Kernel RSP[%lx] "
      "Top[%lx]\n",
      fault_addr, current_thread->Id(), CPUContextManager::GetCurrentCPUId(),
      args->rip, args->rsp, CPURegsAccessProvider::ReadRSP(),
      current_thread->GetKernelStackTop());

  // Kernel thread should not page fault!
  if (current_thread->IsKernelThread()) {
    QemuSerialLog::Logf("#PF in kernel thread! %lx %lx %lx\n", args->rip,
                        args->rsp, CPURegsAccessProvider::ReadCR2());
    PrintStackTrace();
    PANIC();
  }

  // No need to explicitly set here because if the scheduling ever happens,
  // since the RIP is in the kernel space, it will know.
  // process->SetInKernel();

  auto* process_regs = process->GetSavedUserRegs();
  process_regs->regs = *regs;
  CopyCPUInteruptHandlerArgs(process_regs, args);

  auto address_info = process->GetAddressInfo(fault_addr);
  if (address_info == ProcessAddressInfo::NOT_VALID_ADDR) {
    PageTablePrintUtil::PrintUserTable(
        PhysToKernel<uint64_t*>(process->GetPageTableBaseAddress()));

    QemuSerialLog::Logf("Terminate! [fault : %lx] [CPU : %d] rbp : %lx\n",
                        fault_addr, CPUContextManager::GetCurrentCPUId(),
                        regs->rbp);
    PrintStackTrace(regs->rbp);
    // Terminate this process if the fault address is not allowed.
    process->TerminateInInterruptHandler(args, regs);
    return;
  }

  // Must enable interrupt here for the disk access. It is okay to be scheduled
  // within the page fault handler.
  CPURegsAccessProvider::EnableInterrupt();

  // Allocate 1 page.
  uint64_t boundary = Get4KBBoundary(fault_addr);
  AllocatePage(process->GetPageTableBaseAddress(), (uint64_t*)boundary, 0);
  if (address_info == ProcessAddressInfo::ELF_SEGMENT_ADDR) {
    ELFProgramHeader header = process->GetMatchingProgramHeader(fault_addr);

    uint64_t file_read_start_offset =
        header.p_offset + (max(header.p_vaddr, boundary) - header.p_vaddr);
    uint64_t file_read_end_offset =
        header.p_offset +
        min(header.p_filesz, boundary + FourKB - header.p_vaddr);
    uint64_t num_read = file_read_end_offset - file_read_start_offset;

    // If the address was ELF section, then we need to copy it from the file.
    auto& file_system = Ext2FileSystem::GetExt2FileSystem();
    file_system.ReadFile(
        process->GetFileName().c_str(),
        reinterpret_cast<uint8_t*>(max(header.p_vaddr, boundary)), num_read,
        file_read_start_offset);
  }

  /*
  QemuSerialLog::Logf("[CPU %d] Going back to [rip : %lx] [rsp : %lx] \n",
                      CPUContextManager::GetCurrentCPUId(), args->rip,
                      args->rsp);
                      */
  CPURegsAccessProvider::DisableInterrupt();

  // Done handling.
  process->SetInUser();

  TaskStateSegmentManager::GetTaskStateSegmentManager().SetRSP0(
      current_thread->GetKernelStackTop());
}

void PageTableManager::CopyUserPageTable(uint64_t* from_pml4_base_addr,
                                         uint64_t* to_pml4_base_addr) {
  // Copy page table.
  UNUSED(from_pml4_base_addr);
  UNUSED(to_pml4_base_addr);
}
// Print the page table entires where the user can access.
void PageTablePrintUtil::PrintUserTable(uint64_t* cr3) {
  int cpu_id = CPUContextManager::GetCurrentCPUId();
  for (uint64_t i = 0; i < 512; i++) {
    if (IsPresent(cr3[i])) {
      if (IsUserAccessible(cr3[i])) {
        QemuSerialLog::Logf("[%d] PML4 [U] (%lx) ~ (%lx) : [%lx] \n", cpu_id,
                            (i << 39), ((i + 1) << 39) - 1,
                            GetBaseAddress(cr3[i]));
        PrintPDPE(PhysToKernel<uint64_t*>(GetBaseAddress(cr3[i])), i << 39);
      } else {
        QemuSerialLog::Logf("[%d] PML4 [S] (%lx) ~ (%lx) : [%lx] \n", cpu_id,
                            (i << 39), ((i + 1) << 39) - 1,
                            GetBaseAddress(cr3[i]));
      }
    }
  }
}

void PageTablePrintUtil::PrintPDPE(uint64_t* pdpt_top, uint64_t start_addr) {
  int cpu_id = CPUContextManager::GetCurrentCPUId();
  for (uint64_t i = 0; i < 512; i++) {
    if (IsPresent(pdpt_top[i]) && IsUserAccessible(pdpt_top[i])) {
      QemuSerialLog::Logf("> [%d] PDPT (%lx) ~ (%lx) : [%lx] \n", cpu_id,
                          (i << 30) + start_addr,
                          ((i + 1) << 30) - 1 + start_addr,
                          GetBaseAddress(pdpt_top[i]));
      PrintPDE(PhysToKernel<uint64_t*>(GetBaseAddress(pdpt_top[i])),
               (i << 30) + start_addr);
    }
  }
}

void PageTablePrintUtil::PrintPDE(uint64_t* pdt_top, uint64_t start_addr) {
  int cpu_id = CPUContextManager::GetCurrentCPUId();
  for (uint64_t i = 0; i < 512; i++) {
    if (IsPresent(pdt_top[i]) && IsUserAccessible(pdt_top[i])) {
      QemuSerialLog::Logf(">> [%d] PDT (%lx) ~ (%lx) : [%lx] \n", cpu_id,
                          (i << 21) + start_addr,
                          ((i + 1) << 21) - 1 + start_addr,
                          GetBaseAddress(pdt_top[i]));
      PrintPT(PhysToKernel<uint64_t*>(GetBaseAddress(pdt_top[i])),
              (i << 21) + start_addr);
    }
  }
}

void PageTablePrintUtil::PrintPT(uint64_t* pt_top, uint64_t start_addr) {
  int cpu_id = CPUContextManager::GetCurrentCPUId();
  for (uint64_t i = 0; i < 512; i++) {
    if (IsPresent(pt_top[i]) && IsUserAccessible(pt_top[i])) {
      QemuSerialLog::Logf(">>> [%d] PT (%lx) ~ (%lx) : [%lx] \n", cpu_id,
                          (i << 12) + start_addr,
                          ((i + 1) << 12) - 1 + start_addr,
                          GetBaseAddress(pt_top[i]));
    }
  }
}

bool IsKernelMemory(uint64_t memory_addr) {
  return memory_addr >= kKernelVirtualOffset;
}

}  // namespace Kernel

void PageFaultInterruptHandlerCaller(Kernel::CPUInterruptHandlerArgs* args,
                                     Kernel::InterruptHandlerSavedRegs* regs) {
  Kernel::PageTableManager::GetPageTableManager().PageFaultHandler(args, regs);
}
