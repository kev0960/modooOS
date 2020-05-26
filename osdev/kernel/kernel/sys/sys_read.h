#ifndef SYS_SYS_READ_H
#define SYS_SYS_READ_H

#include "../fs/actual_file_desc.h"
#include "../kthread.h"
#include "../process.h"
#include "../qemu_log.h"
#include "../scheduler.h"
#include "sys.h"

namespace Kernel {

class SysReadHandler : public SyscallHandler<SysReadHandler> {
 public:
  size_t SysRead(int fd, char* buf, size_t count) {
    ASSERT(!KernelThread::CurrentThread()->IsKernelThread());
    Process* process = static_cast<Process*>(KernelThread::CurrentThread());

    FileDescriptorTable& table = process->GetFileDescriptorTable();
    FileDescriptor* desc = table.GetDescriptor(fd);

    // fd is not opened.
    if (desc == nullptr) {
      return 0;
    }

    if (desc->GetDescriptorType() == FileDescriptor::ACTUAL_FILE) {
      ActualFileDescriptor* actual_file_desc =
          static_cast<ActualFileDescriptor*>(desc);
      return actual_file_desc->Read(buf, count);
    }

    return 0;
  }
};

}  // namespace Kernel

#endif
