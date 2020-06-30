#ifndef SYS_SYS_MSTICK
#define SYS_SYS_MSTICK

#include "../process.h"
#include "../timer.h"
#include "sys.h"

namespace Kernel {

class SysMsTickHandler : public SyscallHandler<SysMsTickHandler> {
 public:
  size_t SysMsTick() {
    ASSERT(!KernelThread::CurrentThread()->IsKernelThread());
    auto& tm = TimerManager::GetCurrentTimer();
    return tm.GetMsTick();
  }
};

}  // namespace Kernel

#endif
