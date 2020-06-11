#ifndef SYS_SYS_SPAWN_H
#define SYS_SYS_SPAWN_H

#include "../fs/ext2.h"
#include "../kthread.h"
#include "../process.h"
#include "../qemu_log.h"
#include "sys.h"

namespace Kernel {

class SysSpawnHandler : public SyscallHandler<SysSpawnHandler> {
 public:
  int SysSpawn(pid_t* pid, const char* path) {
    ASSERT(!KernelThread::CurrentThread()->IsKernelThread());
    Process* parent = static_cast<Process*>(KernelThread::CurrentThread());

    auto file_path = Ext2FileSystem::GetCanonicalAbsolutePath(
        Ext2FileSystem::GetAbsolutePath(path, parent->GetWorkingDir()));
    QemuSerialLog::Logf("Spawn file : %s \n", file_path.c_str());

    Process* child = ProcessManager::GetProcessManager().CreateProcess(
        file_path.c_str(), parent->GetWorkingDir().c_str());

    QemuSerialLog::Logf("Created process pid: %d \n", child->Id());
    // Returns a non zero error on failure.
    if (child == nullptr) {
      return 1;
    }

    *pid = child->Id();
    child->Start();
    return 0;
  }
};

}  // namespace Kernel

#endif
