#include "timer.h"
#include "stdint.h"
#include "scheduler.h"

namespace Kernel {
PITimer::PITimer() : timer_tick_lower_(0), timer_tick_upper_(0) {}

void PITimer::TimerInterruptHandler() {
  if (timer_tick_lower_ == UINT64_MAX) {
    timer_tick_lower_ = 0;
    timer_tick_upper_++;
  }
  timer_tick_lower_++;

  if (timer_tick_lower_ % 100 == 0) {
    KernelThreadScheduler::GetKernelThreadScheduler().schedule();
  }
}

void PITimer::InstallPITimer() const {
  const uint16_t divisor = 1193180 / PITIMER_HZ;

  outb(PIT_CONTROL, 0x36);
  outb(PIT_1, divisor & 0xFF);  // Lower 1 byte
  outb(PIT_1, divisor >> 8);    // High 1 byte
}

PITimer pic_timer;

}  // namespace Kernel
