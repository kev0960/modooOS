#include "timer.h"
#include "../std/stdint.h"
#include "cpu.h"
#include "kernel_context.h"
#include "kthread.h"
#include "printf.h"
#include "scheduler.h"

namespace Kernel {
PITimer::PITimer() : timer_tick_(0), waiting_thread_sema_(1) {}

void PITimer::TimerInterruptHandler(CPUInterruptHandlerArgs* args,
                                    InterruptHandlerSavedRegs* regs) {
  timer_tick_++;

  // This one should be the last.
  if (timer_tick_ % 50 == 0) {
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

void PITimer::Sleep(uint64_t num_tick) {
  SemaAndEndTime* sema_and_time = nullptr;

  waiting_thread_sema_.Down();

  size_t current_tid = KernelThread::CurrentThread()->Id();
  auto itr = waiting_threads_.find(current_tid);
  if (itr == waiting_threads_.end()) {
    sema_and_time = new SemaAndEndTime(timer_tick_ + num_tick);
    waiting_threads_[current_tid] = sema_and_time;
  } else {
    sema_and_time = itr->second;
    waiting_threads_[current_tid]->timer_tick = timer_tick_ + num_tick;
    waiting_threads_[current_tid]->on = false;
  }

  waiting_thread_sema_.Up();

  sema_and_time->sema.Down();
}

void HandleWaitingThreads() {
  while (true) {
    auto& waiting_threads = pic_timer.WaitingThreads();

    pic_timer.WaitingThreadSema().Down();
    kprintf("%d ", waiting_threads.size());
    while (true) {
      bool is_changed = false;
      for (auto itr = waiting_threads.begin(); itr != waiting_threads.end();
           ++itr) {
        PITimer::SemaAndEndTime* sema_and_time = itr->second;
        if (sema_and_time->timer_tick <= pic_timer.GetClock() &&
            !sema_and_time->on) {
          sema_and_time->on = true;
          sema_and_time->sema.Up();

          // delete sema_and_time;
          is_changed = true;
          break;
        }
      }

      if (!is_changed) {
        break;
      }
    }
    pic_timer.WaitingThreadSema().Up();

    KernelThreadScheduler::GetKernelThreadScheduler().Yield();
  }
}

void PITimer::RegisterAlarmClock() {
  KernelThread* alarm_clock = new KernelThread(HandleWaitingThreads);
  alarm_clock->Start();

  // This thread will never terminate :p
}

PITimer pic_timer;

}  // namespace Kernel

extern "C" void PITimerCaller(Kernel::CPUInterruptHandlerArgs* args,
                              Kernel::InterruptHandlerSavedRegs* regs) {
  Kernel::pic_timer.TimerInterruptHandler(args, regs);
}

