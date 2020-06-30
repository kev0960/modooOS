#ifndef SYS_SYS_PREAD_H
#define SYS_SYS_PREAD_H

#include "../fs/actual_file_desc.h"
#include "../process.h"
#include "sys.h"

namespace Kernel {

class SysPreadHandler : public SyscallHandler<SysPreadHandler> {
 public:
  size_t SysPread(int fd, char* buf, size_t count, off_t offset) {
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
      actual_file_desc->Seek(offset, ActualFileDescriptor::SEEK_SET);
      return actual_file_desc->Read(buf, count);
    } else {
      // Descriptor is not valid or not opened.
      return -1;
    }

    return 0;
  }
};

}  // namespace Kernel

#endif
