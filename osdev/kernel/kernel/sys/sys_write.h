#ifndef SYS_SYS_WRITE_H
#define SYS_SYS_WRITE_H

#include "../console.h"
#include "../fs/actual_file_desc.h"
#include "../pipe.h"
#include "../process.h"
#include "../qemu_log.h"
#include "sys.h"

namespace Kernel {

class SysWriteHandler : public SyscallHandler<SysWriteHandler> {
 public:
  size_t SysWrite(int fd, uint8_t* buf, size_t count) {
    ASSERT(!KernelThread::CurrentThread()->IsKernelThread());
    Process* process = static_cast<Process*>(KernelThread::CurrentThread());

    FileDescriptorTable& table = process->GetFileDescriptorTable();
    FileDescriptor* desc = table.GetDescriptor(fd);

    // TODO This is just a placeholder.
    if (desc == nullptr) {
      QemuSerialLog::Logf("%s", buf);
      KernelConsole::GetKernelConsole().PrintToTerminal(
          reinterpret_cast<char*>(buf), count);
      // kprintf("%s", buf);
    } else if (desc->GetDescriptorType() == FileDescriptor::ACTUAL_FILE) {
      ActualFileDescriptor* actual_file_desc =
          static_cast<ActualFileDescriptor*>(desc);
      return actual_file_desc->Write(buf, count);
    } else if (desc->GetDescriptorType() == FileDescriptor::PIPE_WRITE) {
      PipeDescriptorWriteEnd* write_end =
          static_cast<PipeDescriptorWriteEnd*>(desc);
      return write_end->Write(reinterpret_cast<char*>(buf), count);
    }
    return -1;
  }
};

}  // namespace Kernel

#endif
