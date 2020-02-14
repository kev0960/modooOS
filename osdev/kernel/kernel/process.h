#ifndef PROCESS_H
#define PROCESS_H

#include "../std/string_view.h"
#include "elf.h"
#include "kernel_list.h"
#include "kthread.h"

namespace Kernel {
enum class ProcessAddressInfo {
  NOT_VALID_ADDR,
  STACK_ADDR,
  ELF_SEGMENT_ADDR,
  HEAP_ADDR
};

// Reprsents the user process.
class Process : public KernelThread {
 public:
  // Specify nullptr to parent if it is the process is the first process.
  Process(KernelThread* parent, const KernelString& file_name,
          EntryFuncType entry_function);

  KernelList<Process*>* GetChildrenList() { return &children_; }
  bool IsKernelThread() const override { return false; }
  bool IsInKernelSpace() const override { return in_kernel_space_; }

  uint64_t* GetPageTableBaseAddress() const override {
    return pml4e_base_phys_addr_;
  }

  void SetProgramHeaders(std::vector<ELFProgramHeader> headers) {
    program_headers_ = headers;
  }

  const std::vector<ELFProgramHeader>& GetProgramHeaders() {
    return program_headers_;
  }

  SavedRegisters* GetSavedUserRegs() { return &user_regs_; }
  const KernelString& GetFileName() { return file_name_; }

  void SetInKernel() { in_kernel_space_ = true; }
  void SetInUser() { in_kernel_space_ = false; }

  // Check whether the address falls within
  //   - Current process's stack size.
  //   - ELF segments.
  //   - Allocated heap area.
  // Otherwise it is unrecoverable page fault and the process should be
  // immediately trashed.
  ProcessAddressInfo GetAddressInfo(uint64_t addr) const;
  ELFProgramHeader GetMatchingProgramHeader(uint64_t addr) const;

 private:
  bool in_kernel_space_;
  SavedRegisters user_regs_;

  KernelThread* parent_;

  KernelListElement<Process*> kernel_list_elem_;

  // List of children processes.
  KernelList<Process*> children_;

  uint64_t* pml4e_base_phys_addr_;
  std::vector<ELFProgramHeader> program_headers_;

  KernelString file_name_;
};

class ProcessManager {
 public:
  static ProcessManager& GetProcessManager() {
    static ProcessManager process_manager;
    return process_manager;
  }

  Process* CreateProcess(string_view file_name);

 private:
  ProcessManager() {}
};

}  // namespace Kernel
#endif
