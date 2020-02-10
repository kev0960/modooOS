#include "process.h"

#include "../std/printf.h"
#include "elf.h"
#include "ext2.h"
#include "kernel_math.h"
#include "paging.h"

namespace Kernel {
namespace {

constexpr uint64_t kUserProcessStackAddress = 0x40000000;
constexpr uint64_t kFourKB = (1 << 12);

}  // namespace

// Since this is a user process, it does not need a kernel stack.
Process::Process(KernelThread* parent, const KernelString& file_name,
                 EntryFuncType entry_function)
    : KernelThread(entry_function, /*need_stack=*/false),
      parent_(parent),
      kernel_list_elem_(nullptr),
      file_name_(file_name) {
  if (parent_ != nullptr && !parent_->IsKernelThread()) {
    Process* parent_process = static_cast<Process*>(parent);

    // Push itself to parent process's children list.
    kernel_list_elem_.ChangeList(parent_process->GetChildrenList());
    kernel_list_elem_.PushBack();
  }

  // We need to get a frame for the process.
  auto& page_table_manager = PageTableManager::GetPageTableManager();
  pml4e_base_phys_addr_ = page_table_manager.CreateUserPageTable();

  // Allocate the stack at 0x40000000
  // TODO the stack address is set as arbitrary large number. We need to
  // revisit again and set the value properly.
  PageTableManager::GetPageTableManager().AllocatePage(
      pml4e_base_phys_addr_, (uint64_t*)kUserProcessStackAddress, 0);

  regs_.rsp = kUserProcessStackAddress;
}

ProcessAddressInfo Process::GetAddressInfo(uint64_t addr) const {
  if (kUserProcessStackAddress - kFourKB <= addr &&
      addr <= kUserProcessStackAddress) {
    return ProcessAddressInfo::STACK_ADDR;
  }

  for (const auto header : program_headers_) {
    if (header.p_vaddr <= addr && addr < header.p_vaddr + header.p_memsz) {
      return ProcessAddressInfo::ELF_SEGMENT_ADDR;
    }
  }
  return ProcessAddressInfo::NOT_VALID_ADDR;
}

ELFProgramHeader Process::GetMatchingProgramHeader(uint64_t addr) const {
  for (const auto header : program_headers_) {
    if (header.p_vaddr <= addr && addr < header.p_vaddr + header.p_memsz) {
      return header;
    }
  }

  PANIC();
  return ELFProgramHeader{};
}

Process* ProcessManager::CreateProcess(string_view file_name) {
  auto& ext2_filesystem = Ext2FileSystem::GetExt2FileSystem();
  FileInfo file_info = ext2_filesystem.Stat(file_name);

  // Read the entire file.
  uint8_t* buf = static_cast<uint8_t*>(kmalloc(file_info.file_size));
  ext2_filesystem.ReadFile(file_name, buf, file_info.file_size);

  // Parse the ELF header.
  ELFReader elf_reader(buf, file_info.file_size);

  if (!elf_reader.IsValid()) {
    kprintf("Elf parse error : %s \n", elf_reader.Error().c_str());
    return nullptr;
  }

  // Create a process. Note that the entry function would be the RIP defiend at
  // e_entry ELF header.
  // TODO Optimize following code to only read elf header and program headers.
  const ELFHeader& elf_header = elf_reader.GetHeader();

  Process* process =
      new Process(KernelThread::CurrentThread(), file_name,
                  (KernelThread::EntryFuncType)elf_header.e_entry);

  process->SetProgramHeaders(elf_reader.GetProgramHeaders());
  kfree(buf);

  /*
   for (const ELFProgramHeader& header : process->GetProgramHeaders()) {
     uint64_t vm_boundary = Get4KBBoundary(header.p_vaddr);
     uint64_t needed_page_size = header.p_vaddr + header.p_memsz - vm_boundary;

     PageTableManager::GetPageTableManager().AllocatePage(
         (uint64_t*)process->GetPageTableBaseAddress(), (uint64_t*)vm_boundary,
         RoundUpNearestPowerOfTwoLog(needed_page_size));
   }
   */

  // Now as soon as the kernel switches to this thread, it will first copy the
  // contents from the program headers.
  return process;
}

}  // namespace Kernel
