#ifndef TIMER_H
#define TIMER_H

#include "interrupt.h"
#include "io.h"

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

 private:
  uint64_t timer_tick_lower_;
  uint64_t timer_tick_upper_;
};

extern PITimer pic_timer;

}  // namespace Kernel

// This function will be called in assembly based timer interrupt handler.
extern "C" void PITimerCaller(Kernel::CPUInterruptHandlerArgs* args,
                              Kernel::InterruptHandlerSavedRegs* regs);
#endif
