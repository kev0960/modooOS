#ifndef SYS_SYS_OPEN_H
#define SYS_SYS_OPEN_H

#include "../fs/actual_file_desc.h"
#include "../process.h"
#include "sys.h"

namespace Kernel {

class SysOpenHandler : public SyscallHandler<SysOpenHandler> {
 public:
  int SysOpen(const char* pathname) {
    ASSERT(!KernelThread::CurrentThread()->IsKernelThread());
    Process* process = static_cast<Process*>(KernelThread::CurrentThread());

    auto absolute_path =
        Ext2FileSystem::GetAbsolutePath(pathname, process->GetWorkingDir());
    QemuSerialLog::Logf("Open : %s \n", absolute_path.c_str());

    ActualFileDescriptor* desc = new ActualFileDescriptor(
        Ext2FileSystem::GetExt2FileSystem().GetInodeNumberFromPath(
            absolute_path.c_str()));

    FileDescriptorTable& table = process->GetFileDescriptorTable();
    return table.AddDescriptor(desc);
  }
};

}  // namespace Kernel

#endif
