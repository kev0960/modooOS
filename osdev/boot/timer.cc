#include "timer.h"
#include "cpu.h"
#include "printf.h"
#include "scheduler.h"
#include "stdint.h"

namespace Kernel {
PITimer::PITimer() : timer_tick_lower_(0), timer_tick_upper_(0) {}

void PITimer::TimerInterruptHandler(CPUInterruptHandlerArgs* args,
                                    InterruptHandlerSavedRegs* regs) {
  if (timer_tick_lower_ == UINT64_MAX) {
    timer_tick_lower_ = 0;
    timer_tick_upper_++;
  }
  timer_tick_lower_++;

  // This one should be the last.
  if (timer_tick_lower_ % 2 == 0) {
    KernelThreadScheduler::GetKernelThreadScheduler().YieldInInterruptHandler(
        args, regs);
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

extern "C" void PITimerCaller(Kernel::CPUInterruptHandlerArgs* args,
                              Kernel::InterruptHandlerSavedRegs* regs) {
  // kprintf("rax: %lx \n", regs->rax);
  /*
  kprintf("rbp: %lx \n", regs->rbp);
  kprintf("rax: %x \n", regs->rax);
  kprintf("rbx: %x \n", regs->rcx);
  kprintf("rsi: %x \n", regs->rsi);
  kprintf("rdi: %x \n", regs->rdi);
  while(1);*/
  /*
  kprintf("rflags: %x \n", args->rflags);
  kprintf("rsp: %x \n", args->rsp);
  kprintf("ss: %x \n", args->ss);*/

  Kernel::pic_timer.TimerInterruptHandler(args, regs);
}

