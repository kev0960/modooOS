#include "timer.h"

#include "../std/stdint.h"
#include "acpi.h"
#include "apic.h"
#include "cpu.h"
#include "kernel_context.h"
#include "kthread.h"
#include "printf.h"
#include "qemu_log.h"
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
  if (timer_tick_ % 100 == 0) {
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

void Timer::SleepMs(uint64_t ms) {
  // 10 nano = 10^-8
  // 1 tick = (num_10nanosec_per_tick_) * 10^-8 seconds.
  // Num tick required = ms / 1 tick
  //                   = ms * (10^-3s) / (num_10nanosec_per_tick_* 10^-8)
  //                   = ms * 10^5 / num_10nanosec_per_tick_
  while (!calibration_done_) {
  }

  Sleep(ms * 100000 / num_10nanosec_per_tick_);
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
          // QemuSerialLog::Logf("Sema up\n");
          sema_and_time->sema.Up();

          is_changed = true;
          break;
        }
      }

      if (!is_changed) {
        break;
      }
    }
    // QemuSerialLog::Logf("Waiting Sema up\n");
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

  // Divide by 16
  m.SetRegister(kTimerDivideConfig, 0b0001);
}

uint64_t Timer::Calibrate() {
  uint8_t* hpet_reg_addr = ACPIManager::GetACPIManager().GetHPETRegAddr();
  uint32_t clock_period = *(uint32_t*)(hpet_reg_addr + 4);

  // Default setting : tick per 10^-8 seconds.
  QemuSerialLog::Logf("Clock period %d \n", clock_period);

  uint64_t* config_register = reinterpret_cast<uint64_t*>(hpet_reg_addr + 0x10);

  /*
  uint64_t* timer_config_reg = GetTimerConfigRegister(0);
  int ioapic_intr = 2;
  APICManager::GetAPICManager().RedirectIRQs(
      ioapic_intr, CPUContextManager::GetCurrentCPUId());
  *timer_config_reg =
      (*timer_config_reg) | (ioapic_intr << 9) | (1 << 2);

  uint64_t* timer_comparator_reg = GetTimerComparatorRegister(0);
  QemuSerialLog::Logf("HPET Main coutn : %lx \n", GetHPETMainCount());
  *timer_comparator_reg = *timer_comparator_reg | (GetHPETMainCount() + 10000);

  calibrate_done_ = false;
  */

  // Enable Timer.
  *config_register = 1;

  constexpr int kNumCalibrate = 50;
  constexpr int kCalibrateTicks = 5;

  uint64_t diffs[kNumCalibrate];
  for (int i = 0; i < kNumCalibrate; i++) {
    uint64_t start = GetHPETMainCount();
    Sleep(kCalibrateTicks);
    diffs[i] = GetHPETMainCount() - start;
  }

  // Drop max and min.
  uint64_t max_diff = diffs[0], min_diff = diffs[0];
  int max_index = 0, min_index = 0;
  for (int i = 1; i < kNumCalibrate; i++) {
    if (diffs[i] > max_diff) {
      max_diff = diffs[i];
      max_index = i;
    }
    if (diffs[i] < min_diff) {
      min_diff = diffs[i];
      min_index = i;
    }
  }

  uint64_t total = 0;
  for (int i = 0; i < kNumCalibrate; i++) {
    if (i != max_index && i != min_index) {
      total += diffs[i];
    }
  }

  kprintf("10^-8 seconds per tick : %d \n",
          total / (kNumCalibrate - 2) / kCalibrateTicks);
  num_10nanosec_per_tick_ = total / (kNumCalibrate - 2) / kCalibrateTicks;

  *config_register = 0;
  MarkCalibrationDone();

  return num_10nanosec_per_tick_;
}

uint64_t Timer::GetHPETMainCount() {
  uint8_t* hpet_reg_addr = ACPIManager::GetACPIManager().GetHPETRegAddr();
  return *(uint64_t*)(hpet_reg_addr + 0xF0);
}

uint64_t* Timer::GetTimerConfigRegister(int timer_index) {
  uint8_t* hpet_reg_addr = ACPIManager::GetACPIManager().GetHPETRegAddr();
  return (uint64_t*)(hpet_reg_addr + (0x100 + 0x20 * timer_index));
}

uint64_t* Timer::GetTimerComparatorRegister(int timer_index) {
  uint8_t* hpet_reg_addr = ACPIManager::GetACPIManager().GetHPETRegAddr();
  return (uint64_t*)(hpet_reg_addr + (0x108 + 0x20 * timer_index));
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

void TimerManager::Calibrate() {
  uint64_t num_10nanosec_per_tick = timers_[0].Calibrate();
  for (size_t i = 1; i < timers_.size(); i++) {
    timers_[i].SetNum10NanoSecPerTick(num_10nanosec_per_tick);
    timers_[i].MarkCalibrationDone();
  }
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

