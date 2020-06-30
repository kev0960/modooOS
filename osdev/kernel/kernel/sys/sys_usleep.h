#ifndef SYS_SYS_USLEEP_H
#define SYS_SYS_USLEEP_H

#include "../process.h"
#include "../timer.h"
#include "sys.h"

namespace Kernel {

class SysUSleepHandler : public SyscallHandler<SysUSleepHandler> {
 public:
  size_t SysUSleep(size_t usec) {
    ASSERT(!KernelThread::CurrentThread()->IsKernelThread());
    auto& tm = TimerManager::GetCurrentTimer();

    // TODO Add microsecond sleeping timer.
    tm.SleepMs(usec / 1000);

    return 0;
  }
};

}  // namespace Kernel

#endif
