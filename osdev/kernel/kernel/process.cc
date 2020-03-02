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
    : KernelThread(nullptr, /*need_stack=*/true),
      in_kernel_space_(false),
      parent_(parent),
      kernel_list_elem_(nullptr),
      file_name_(file_name) {
  kprintf("Create process");
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
      pml4e_base_phys_addr_, (uint64_t*)(kUserProcessStackAddress - kFourKB),
      0);

  user_regs_.rip = (uint64_t)entry_function;
  user_regs_.rsp = kUserProcessStackAddress - 8;
  user_regs_.cs = 0x23;       // User Code segment
  user_regs_.ss = 0x1b;       // User Stack segment.
  user_regs_.rflags = 0x200;  // Interrupt is enabled.
}

Process::Process(Process* parent)
    : KernelThread(nullptr, /*need_stack=*/true),
      in_kernel_space_(false),
      parent_(parent),
      kernel_list_elem_(nullptr),
      file_name_(parent->file_name_) {
  kprintf("Cloning the process!");
  kernel_list_elem_.ChangeList(parent->GetChildrenList());
  kernel_list_elem_.PushBack();

  // We need to get a frame for the process.
  auto& page_table_manager = PageTableManager::GetPageTableManager();
  pml4e_base_phys_addr_ = page_table_manager.CreateUserPageTable();

  PageTableManager::GetPageTableManager().AllocatePage(
      pml4e_base_phys_addr_, (uint64_t*)(kUserProcessStackAddress - kFourKB),
      0);

  // Copy the current state of the parent.
  user_regs_ = *parent->GetSavedUserRegs();

  // Specify the return value (0 to the child process for fork()) to the newly
  // created process.
  user_regs_.regs.rax = 0;
}

ProcessAddressInfo Process::GetAddressInfo(uint64_t addr) const {
  if (addr == 0) {
    return ProcessAddressInfo::NOT_VALID_ADDR;
  }

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

  // Now as soon as the kernel switches to this thread, it will first copy the
  // contents from the program headers.
  return process;
}

}  // namespace Kernel
