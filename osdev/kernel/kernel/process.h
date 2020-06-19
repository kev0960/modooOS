#ifndef PROCESS_H
#define PROCESS_H

#include "../std/string_view.h"
#include "elf.h"
#include "file_descriptor.h"
#include "kernel_list.h"
#include "kthread.h"

namespace Kernel {
enum class ProcessAddressInfo {
  NOT_VALID_ADDR,
  STACK_ADDR,
  ELF_SEGMENT_ADDR,
  HEAP_ADDR,
};

// Reprsents the user process.
class Process : public KernelThread {
 public:
  // Address of the top of the stack.
  static constexpr uint64_t kUserProcessStackAddress = 0x40000000;

  // Adress of the start of the heap.
  static constexpr uint64_t kUserProcessHeapStartAddress = 0x10000000;

  // Specify nullptr to parent if it is the process is the first process.
  Process(KernelThread* parent, const KernelString& file_name,
          EntryFuncType entry_function, std::string_view working_dir);

  KernelList<Process*>* GetChildrenList() { return &children_; }
  void SetParent(KernelThread* parent) { parent_ = parent; }
  KernelThread* GetParent() { return parent_; }

  bool IsKernelThread() const override { return false; }
  bool IsInKernelSpace() const override { return in_kernel_space_; }

  uint64_t* GetPageTableBaseAddress() const override {
    return pml4e_base_phys_addr_;
  }

  void SetProgramHeaders(std::vector<ELFProgramHeader> headers) {
    program_headers_ = headers;
  }

  void SetSectionHeaders(std::vector<ELFSectionHeader> headers) {
    section_headers_ = headers;
  }

  const std::vector<ELFProgramHeader>& GetProgramHeaders() {
    return program_headers_;
  }

  SavedRegisters* GetSavedUserRegs() { return &user_regs_; }
  const KernelString& GetFileName() { return file_name_; }

  void SetInKernel() { in_kernel_space_ = true; }
  void SetInUser() { in_kernel_space_ = false; }

  KernelListElement<Process*>* GetChildListElem() { return &child_list_elem_; }

  void SetExitCode(pid_t pid, uint64_t exit_code);

  // Read exit code of the child process pid. If success, returns true and sets
  // the exit_code. Otherwise, returns false.
  bool ReadExitCode(pid_t pid, uint64_t* exit_code);

  FileDescriptorTable& GetFileDescriptorTable() { return fd_table_; }

  // Check whether the address falls within
  //   - Current process's stack size.
  //   - ELF segments.
  //   - Allocated heap area.
  // Otherwise it is unrecoverable page fault and the process should be
  // immediately trashed.
  ProcessAddressInfo GetAddressInfo(uint64_t addr) const;
  ELFProgramHeader GetMatchingProgramHeader(uint64_t addr) const;

  std::vector<KernelString>& GetArgv() { return argv_; }
  void CopyArgvToStack();

  int GetNumPageFault() const { return num_page_fault_; }
  void IncNumPageFault() { num_page_fault_++; }

  KernelString GetWorkingDir() const { return working_dir_; }

  // Increase size of the process heap by (bytes) bytes.
  // It must be multiple of 4KB (Page size).
  void IncreaseHeapSize(uint64_t bytes);
  void* GetHeapEnd() const;

  void ZeroInitIfNeeded(uint64_t boundary);

 private:
  bool in_kernel_space_;
  SavedRegisters user_regs_;

  KernelThread* parent_;

  // Element that belongs to the parent's process.
  KernelListElement<Process*> child_list_elem_;

  // List of children processes.
  KernelList<Process*> children_;

  uint64_t* pml4e_base_phys_addr_;
  std::vector<ELFProgramHeader> program_headers_;
  std::vector<ELFSectionHeader> section_headers_;

  KernelString file_name_;

  FileDescriptorTable fd_table_;

  // Map children's pid_t to the exit code.
  MultiCoreSpinLock exit_code_lock_;
  std::map<pid_t, int> child_to_exit_code_;

  std::vector<KernelString> argv_;

  int num_page_fault_;

  KernelString working_dir_;

  // Heap size.
  uint64_t heap_size_;
};

class ProcessManager {
 public:
  static ProcessManager& GetProcessManager() {
    static ProcessManager process_manager;
    return process_manager;
  }

  Process* CreateProcess(std::string_view file_name,
                         std::string_view working_dir);
  Process* CreateProcess(std::string_view file_name,
                         std::string_view working_dir,
                         std::vector<KernelString> argv);

 private:
  ProcessManager() {}
};

}  // namespace Kernel
#endif
