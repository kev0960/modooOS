#ifndef SYS_SYS_LSEEK_H
#define SYS_SYS_LSEEK_H

#include "../fs/actual_file_desc.h"
#include "../process.h"
#include "sys.h"

namespace Kernel {

namespace {

ActualFileDescriptor::Whence IntToWhence(int whence) {
  switch (whence) {
    case 0:
      return ActualFileDescriptor::SEEK_SET;
    case 1:
      return ActualFileDescriptor::SEEK_CUR;
    case 2:
      return ActualFileDescriptor::SEEK_END;
  }

  return ActualFileDescriptor::SEEK_SET;
}

}  // namespace

class SysLseekHandler : public SyscallHandler<SysLseekHandler> {
 public:
  size_t SysLseek(int fd, off_t offset, int whence) {
    ASSERT(!KernelThread::CurrentThread()->IsKernelThread());
    Process* process = static_cast<Process*>(KernelThread::CurrentThread());

    FileDescriptorTable& table = process->GetFileDescriptorTable();
    FileDescriptor* desc = table.GetDescriptor(fd);

    // fd is not opened.
    if (desc == nullptr) {
      return -1;
    }

    if (desc->GetDescriptorType() == FileDescriptor::ACTUAL_FILE) {
      ActualFileDescriptor* actual_file_desc =
          static_cast<ActualFileDescriptor*>(desc);
      return actual_file_desc->Seek(offset, IntToWhence(whence));
    }

    return -1;
  }
};

}  // namespace Kernel

#endif
