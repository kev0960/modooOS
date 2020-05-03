#include "timer.h"

#include "../std/stdint.h"
#include "apic.h"
#include "cpu.h"
#include "kernel_context.h"
#include "kthread.h"
#include "printf.h"
#include "scheduler.h"

namespace Kernel {
namespace {

constexpr uint32_t kTimerLocalVectorTable = 0x320;
constexpr uint32_t kTimerInitialCount = 0x380;
constexpr uint32_t kTimerDivideConfig = 0x3E0;

}  // namespace

Timer::Timer(int timer_id)
    : timer_tick_(0), waiting_thread_sema_(1), timer_id_(timer_id) {}

void Timer::TimerInterruptHandler(CPUInterruptHandlerArgs* args,
                                  InterruptHandlerSavedRegs* regs) {
  timer_tick_++;

  if (APICManager::GetAPICManager().IsMulticoreEnabled()) {
    APICManager::GetAPICManager().SetEndOfInterrupt();
  }

  // This one should be the last.
  if (timer_tick_ % 30 == 0) {
    KernelThreadScheduler::GetKernelThreadScheduler().YieldInInterruptHandler(
        args, regs);
  }
}

void Timer::InstallTimer() const {
  const uint16_t divisor = 1193180 / PITIMER_HZ;

  outb(PIT_CONTROL, 0x36);
  outb(PIT_1, divisor & 0xFF);  // Lower 1 byte
  outb(PIT_1, divisor >> 8);    // High 1 byte
}

void Timer::Sleep(uint64_t num_tick) {
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
    auto& timer = TimerManager::GetTimerManager().GetTimer();
    auto& waiting_threads = timer.WaitingThreads();

    timer.WaitingThreadSema().Down();
    while (true) {
      bool is_changed = false;
      for (auto itr = waiting_threads.begin(); itr != waiting_threads.end();
           ++itr) {
        Timer::SemaAndEndTime* sema_and_time = itr->second;
        if (sema_and_time->timer_tick <= timer.GetClock() &&
            !sema_and_time->on) {
          sema_and_time->on = true;
          sema_and_time->sema.Up();

          is_changed = true;
          break;
        }
      }

      if (!is_changed) {
        break;
      }
    }
    timer.WaitingThreadSema().Up();

    KernelThreadScheduler::GetKernelThreadScheduler().Yield();
  }
}

void Timer::RegisterAlarmClock() {
  KernelThread* alarm_clock = new KernelThread(HandleWaitingThreads);
  alarm_clock->Start();

  // This thread will never terminate :p
}

void Timer::StartAPICTimer() {
  // kprintf("Start apic! (%d)", timer_id_);
  auto& m = APICManager::GetAPICManager();
  m.SetRegister(kTimerInitialCount, 0x1FFFF);
  m.SetRegister(kTimerLocalVectorTable, (1 << 17) | 0x20);

  // Divide by 128
  m.SetRegister(kTimerDivideConfig, 0b1010);
}

TimerManager::TimerManager() { timers_.push_back(Timer(0)); }

void TimerManager::InstallPICTimer() {
  ASSERT(timers_.size() == 1);
  timers_[0].InstallTimer();
}

void TimerManager::InstallAPICTimer(int num_cores) {
  timers_.reserve(num_cores);

  // Since we already have a timer for BSP, just need to add timers for APs.
  for (int i = 0; i < num_cores - 1; i++) {
    timers_.push_back(Timer(i + 1));
  }
}

void TimerManager::StartAPICTimer() {
  timers_[CPUContextManager::GetCPUContextManager().GetCPUContext()->cpu_id]
      .StartAPICTimer();
}

void TimerManager::RegisterAlarmClock() {
  timers_[CPUContextManager::GetCPUContextManager().GetCPUContext()->cpu_id]
      .RegisterAlarmClock();
}

Timer& TimerManager::GetTimer() {
  if (timers_.size() == 1) {
    return timers_[0];
  }

  // Otherwise, get the current CPU context and return the appropriate timer.
  return timers_
      [CPUContextManager::GetCPUContextManager().GetCPUContext()->cpu_id];
}

}  // namespace Kernel

extern "C" void TimerCaller(Kernel::CPUInterruptHandlerArgs* args,
                            Kernel::InterruptHandlerSavedRegs* regs) {
  auto& timer = Kernel::TimerManager::GetTimerManager().GetTimer();
  int cpu_id =
      Kernel::CPUContextManager::GetCPUContextManager().GetCPUContext()->cpu_id;
  if (cpu_id == 0) {
    // kprintf("[%d] ", cpu_id);
  }
  timer.TimerInterruptHandler(args, regs);
}

