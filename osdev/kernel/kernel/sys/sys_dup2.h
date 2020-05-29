#ifndef SYS_SYS_DUP2_H
#define SYS_SYS_DUP2_H

#include "../fs/actual_file_desc.h"
#include "../pipe.h"
#include "../process.h"
#include "sys.h"

namespace Kernel {

class SysDup2Handler : public SyscallHandler<SysDup2Handler> {
 public:
  size_t SysDup2(int oldfd, int newfd) {
    ASSERT(!KernelThread::CurrentThread()->IsKernelThread());
    Process* process = static_cast<Process*>(KernelThread::CurrentThread());

    FileDescriptorTable& table = process->GetFileDescriptorTable();
    FileDescriptor* desc = table.GetDescriptor(oldfd);

    FileDescriptor* prev = table.SetDescriptor(newfd, desc);
    if (prev != nullptr) {
      prev->RemoveProcess(process->Id());
    }

    return 0;
  }
};

}  // namespace Kernel

#endif
