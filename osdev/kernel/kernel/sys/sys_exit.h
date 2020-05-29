#ifndef SYS_SYS_EXIT_H
#define SYS_SYS_EXIT_H

#include "../kthread.h"
#include "../process.h"
#include "sys.h"

namespace Kernel {

class SysExitHandler : public SyscallHandler<SysExitHandler> {
 public:
  int SysExit(uint64_t exit_code) {
    ASSERT(!KernelThread::CurrentThread()->IsKernelThread());
    Process* process = static_cast<Process*>(KernelThread::CurrentThread());

    KernelThread* parent = process->GetParent();
    if (parent != nullptr && !parent->IsKernelThread()) {
      static_cast<Process*>(parent)->SetExitCode(process->Id(), exit_code);
    }
    // kprintf("Terminate thread : %d %d\n", exit_num, current_thread->Id());

    // Let descriptors know that this process is now being terminated.
    // It will probably remove descriptors that are not being used anymore.
    process->GetFileDescriptorTable().RemoveProcessIdToDescriptors(
        process->Id());
    process->Terminate();

    return exit_code;
  }
};

}  // namespace Kernel

#endif
