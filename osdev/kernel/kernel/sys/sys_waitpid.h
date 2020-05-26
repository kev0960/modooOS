#ifndef SYS_SYS_WAITPID_H
#define SYS_SYS_WAITPID_H

#include "../kthread.h"
#include "../process.h"
#include "../scheduler.h"
#include "sys.h"

namespace Kernel {

class SysWaitpidHandler : public SyscallHandler<SysWaitpidHandler> {
 public:
  pid_t SysWaitpid(pid_t pid, int* status) {
    ASSERT(!KernelThread::CurrentThread()->IsKernelThread());
    Process* process = static_cast<Process*>(KernelThread::CurrentThread());

    // Loop until child process finishes.
    // TODO Implement it via semaphore to avoid busy waiting?
    while (!process->ReadExitCode(pid, reinterpret_cast<uint64_t*>(status))) {
      KernelThreadScheduler::GetKernelThreadScheduler().Yield();
    }

    return pid;
  }
};

}  // namespace Kernel

#endif
