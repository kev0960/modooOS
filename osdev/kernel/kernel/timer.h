#ifndef TIMER_H
#define TIMER_H

#include "../std/vector.h"
#include "interrupt.h"
#include "io.h"
#include "kthread.h"

// PIT Data channels.
#define PIT_1 0x40
#define PIT_2 0x41
#define PIT_3 0x42

#define PIT_CONTROL 0x43
#define PITIMER_HZ 100

namespace Kernel {

class PITimer {
 public:
  PITimer();

  void TimerInterruptHandler(CPUInterruptHandlerArgs* args,
                             InterruptHandlerSavedRegs* regs);

  // Install PIT.
  void InstallPITimer() const;

  uint64_t GetClock() const { return timer_tick_lower_; }

  void Sleep();

 private:
  uint64_t timer_tick_lower_;

  // Just ignore upper tick :p. At tick per 0.01 seconds, we will need
  // 1844674407370955.16 seconds to hit this upper tick. This is roughly 58
  // million years!
  uint64_t timer_tick_upper_;

  struct SemaAndEndTime {
    uint64_t timer_tick;
    Semaphore sema;
  };

  std::vector<SemaAndEndTime> waiting_threads_;
};

extern PITimer pic_timer;

}  // namespace Kernel

// This function will be called in assembly based timer interrupt handler.
extern "C" void PITimerCaller(Kernel::CPUInterruptHandlerArgs* args,
                              Kernel::InterruptHandlerSavedRegs* regs);
#endif
