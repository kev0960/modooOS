#ifndef SYS_SYS_STAT_H
#define SYS_SYS_STAT_H

#include "../fs/ext2.h"
#include "../process.h"
#include "sys.h"

namespace Kernel {

class SysStatHandler : public SyscallHandler<SysStatHandler> {
 public:
  int SysStat(const char* filename, FileInfo* file_info) {
    ASSERT(!KernelThread::CurrentThread()->IsKernelThread());
    Process* process = static_cast<Process*>(KernelThread::CurrentThread());

    auto absolute_path =
        Ext2FileSystem::GetAbsolutePath(filename, process->GetWorkingDir());
    QemuSerialLog::Logf("Stat : %s \n", absolute_path.c_str());

    FileInfo info =
        Ext2FileSystem::GetExt2FileSystem().Stat(absolute_path.c_str());
    if (info.inode == 0 && info.file_size == 0) {
      return -1;
    }

    *file_info = info;
    return 0;
  }
};

}  // namespace Kernel

#endif
