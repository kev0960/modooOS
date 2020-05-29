
#ifndef SYS_SYS_PIPE_H
#define SYS_SYS_PIPE_H

#include "../pipe.h"
#include "../process.h"
#include "../qemu_log.h"
#include "sys.h"

namespace Kernel {

class SysPipeHandler : public SyscallHandler<SysPipeHandler> {
 public:
  int SysPipe(int fd[2]) {
    ASSERT(!KernelThread::CurrentThread()->IsKernelThread());
    Process* process = static_cast<Process*>(KernelThread::CurrentThread());

    Pipe* pipe = new Pipe();
    PipeDescriptorReadEnd* read_end = new PipeDescriptorReadEnd(pipe);
    PipeDescriptorWriteEnd* write_end = new PipeDescriptorWriteEnd(pipe);

    FileDescriptorTable& table = process->GetFileDescriptorTable();

    fd[0] = table.AddDescriptor(read_end);
    fd[1] = table.AddDescriptor(write_end);
    return 0;
  }
};

}  // namespace Kernel

#endif
