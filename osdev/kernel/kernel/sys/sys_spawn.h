#ifndef SYS_SYS_SPAWN_H
#define SYS_SYS_SPAWN_H

#include "../kthread.h"
#include "../process.h"
#include "../qemu_log.h"
#include "sys.h"

namespace Kernel {

class SysSpawnHandler : public SyscallHandler<SysSpawnHandler> {
 public:
  int SysSpawn(pid_t* pid, const char* path) {
    Process* process = ProcessManager::GetProcessManager().CreateProcess(path);

    QemuSerialLog::Logf("Created process pid: %d \n", process->Id());
    // Returns a non zero error on failure.
    if (process == nullptr) {
      return 1;
    }

    *pid = process->Id();
    process->Start();
    return 0;
  }
};

}  // namespace Kernel

#endif
