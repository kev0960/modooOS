#ifndef SYS_SYS_STAT_H
#define SYS_SYS_STAT_H

#include "../fs/ext2.h"
#include "sys.h"

namespace Kernel {

class SysStatHandler : public SyscallHandler<SysStatHandler> {
 public:
  int SysStat(const char* filename, FileInfo* file_info) {
    ASSERT(!KernelThread::CurrentThread()->IsKernelThread());

    FileInfo info = Ext2FileSystem::GetExt2FileSystem().Stat(filename);
    if (info.inode == 0 && info.file_size == 0) {
      return -1;
    }

    *file_info = info;
    return 0;
  }
};

}  // namespace Kernel

#endif
