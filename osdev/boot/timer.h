#ifndef TIMER_H
#define TIMER_H

#include "io.h"

// PIT Data channels.
#define PIT_1 0x40
#define PIT_2 0x41
#define PIT_3 0x42

#define PIT_CONTROL 0x43

namespace Kernel {

class PITimer {
 public:
  PITimer();

  void TimerInterruptHandler();

  // Install PIT.
  void InstallPITimer(uint32_t hz) const;

  uint64_t GetClock() const {
    return timer_tick_lower_;
  }

 private:
  uint64_t timer_tick_lower_;
  uint64_t timer_tick_upper_;
};

extern PITimer pic_timer;

}  // namespace Kernel

#endif
