#include "process.h"

#include "./fs/ext2.h"
#include "cpu_context.h"
#include "elf.h"
#include "kernel_math.h"
#include "paging.h"
#include "qemu_log.h"

namespace Kernel {
namespace {

constexpr uint64_t kFourKB = (1 << 12);

uint64_t CopyStringToStack(const KernelString& s, uint64_t rsp) {
  // We have to put NULL terminator too.
  rsp = rsp - (s.size() + 1);

  for (size_t i = 0; i < s.size(); i++) {
    *reinterpret_cast<char*>(rsp + i) = s.at(i);
  }
  *reinterpret_cast<char*>(rsp + s.size()) = 0;

  return rsp;
}

template <typename T>
uint64_t WriteToRSP(T val, uint64_t rsp) {
  rsp -= sizeof(T);
  *reinterpret_cast<T*>(rsp) = val;
  return rsp;
}

}  // namespace

Process::Process(KernelThread* parent, const KernelString& file_name,
                 EntryFuncType entry_function, std::string_view working_dir)
    : KernelThread(nullptr, /*need_stack=*/true, /*in_same_cpu_id=*/false),
      in_kernel_space_(false),
      parent_(parent),
      child_list_elem_(nullptr),
      file_name_(file_name),
      num_page_fault_(0),
      working_dir_(working_dir),
      heap_size_(0) {
  child_list_elem_.Set(this);

  if (parent_ != nullptr && !parent_->IsKernelThread()) {
    Process* parent_process = static_cast<Process*>(parent);

    // Push itself to parent process's children list.
    child_list_elem_.ChangeList(parent_process->GetChildrenList());
    child_list_elem_.PushBack();

    // Copy the descriptor table.
    fd_table_ = parent_process->fd_table_;
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

  // Make sure the descriptors recognizes this process.
  fd_table_.AddProcessIdToDescriptors(Id());
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

void Process::CopyArgvToStack() {
  ASSERT(user_regs_.rsp == kUserProcessStackAddress - 8);

  std::vector<char*> addrs(argv_.size());
  for (int i = argv_.size() - 1; i >= 0; i--) {
    user_regs_.rsp = CopyStringToStack(argv_[i], user_regs_.rsp);
    addrs[i] = reinterpret_cast<char*>(user_regs_.rsp);
  }

  // Align RSP to 8 byte addr boundary.
  user_regs_.rsp = user_regs_.rsp - (user_regs_.rsp % 8);

  // Now copy address table.
  for (int i = addrs.size() - 1; i >= 0; i--) {
    user_regs_.rsp = WriteToRSP(addrs[i], user_regs_.rsp);
  }

  // Copy the number of args.
  user_regs_.rsp = WriteToRSP(argv_.size(), user_regs_.rsp);
}

Process* ProcessManager::CreateProcess(std::string_view file_name,
                                       std::string_view working_dir) {
  auto& ext2_filesystem = Ext2FileSystem::GetExt2FileSystem();
  QemuSerialLog::Logf("File : %s \n", KernelString(file_name).c_str());
  FileInfo file_info = ext2_filesystem.Stat(file_name);

  // File is not found.
  if (file_info.file_size == 0) {
    QemuSerialLog::Logf("not found\n");
    return nullptr;
  }

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
                  (KernelThread::EntryFuncType)elf_header.e_entry, working_dir);

  process->SetProgramHeaders(elf_reader.GetProgramHeaders());
  kfree(buf);

  // Now as soon as the kernel switches to this thread, it will first copy the
  // contents from the program headers.
  return process;
}

void Process::IncreaseHeapSize(uint64_t bytes) {
  ASSERT(bytes % kFourKB == 0);
  int num_pages = bytes / kFourKB;

  auto& page_table_manager = PageTableManager::GetPageTableManager();
  for (int i = 0; i < num_pages; i++) {
    uint64_t* user_vm_addr = reinterpret_cast<uint64_t*>(
        kUserProcessHeapStartAddress + heap_size_ + i * kFourKB);
    page_table_manager.AllocatePage(GetPageTableBaseAddress(), user_vm_addr, 0);
  }

  heap_size_ += bytes;
}

void* Process::GetHeapEnd() const {
  return reinterpret_cast<void*>(kUserProcessHeapStartAddress + heap_size_);
}

Process* ProcessManager::CreateProcess(std::string_view file_name,
                                       std::string_view working_dir,
                                       std::vector<KernelString> argv) {
  Process* process = CreateProcess(file_name, working_dir);
  if (process == nullptr) {
    return nullptr;
  }

  process->GetArgv() = argv;
  return process;
}

void Process::SetExitCode(pid_t pid, uint64_t exit_code) {
  std::lock_guard<MultiCoreSpinLock> lk(exit_code_lock_);
  child_to_exit_code_[pid] = exit_code;
}

bool Process::ReadExitCode(pid_t pid, uint64_t* exit_code) {
  std::lock_guard<MultiCoreSpinLock> lk(exit_code_lock_);
  auto itr = child_to_exit_code_.find(pid);
  if (itr != child_to_exit_code_.end()) {
    *exit_code = (*itr).second;
    return true;
  }
  return false;
}

}  // namespace Kernel
