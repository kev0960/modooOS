#ifndef TIMER_H
#define TIMER_H

#include "../std/map.h"
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

class Timer {
 public:
  Timer(int timer_id);

  void TimerInterruptHandler(CPUInterruptHandlerArgs* args,
                             InterruptHandlerSavedRegs* regs);

  // Install PIT.
  void InstallTimer() const;

  uint64_t GetClock() const { return timer_tick_; }

  void Sleep(uint64_t num_tick);
  void SleepMs(uint64_t ms);

  // This is a thread that wakes up the threads in the waiting_threads_.
  // We cannot wake up threads in the timer handler because it is called in an
  // interrupted context. If the handler fails to acquire lock for
  // waiting_threads_, the deadlock can happen.
  void RegisterAlarmClock();

  Semaphore& WaitingThreadSema() { return waiting_thread_sema_; }

  struct SemaAndEndTime {
    uint64_t timer_tick;
    Semaphore sema;
    bool on;

    SemaAndEndTime(uint64_t timer_tick)
        : timer_tick(timer_tick), sema(0), on(false) {}
  };

  std::map<size_t, SemaAndEndTime*>& WaitingThreads() {
    return waiting_threads_;
  }

  uint64_t Calibrate();
  void StartAPICTimer();
  int GetTimerId() const { return timer_id_; }

  void SetNum10NanoSecPerTick(uint64_t ten_ns) {
    num_10nanosec_per_tick_ = ten_ns;
  }

  void MarkCalibrationDone() { calibration_done_ = true; }

 private:
  // At tick per 0.01 seconds, we will need 1844674407370955.16 seconds to make
  // this overflow. This is roughly 58 million years!
  uint64_t timer_tick_;

  // size_t : Thread id.
  std::map<size_t, SemaAndEndTime*> waiting_threads_;
  Semaphore waiting_thread_sema_;

  int timer_id_;

  uint64_t* GetTimerConfigRegister(int timer_index);
  uint64_t* GetTimerComparatorRegister(int timer_index);
  uint64_t GetHPETMainCount();

  uint64_t num_10nanosec_per_tick_ = 0;
  volatile bool calibration_done_ = false;
};

// Class that manages timers. There should be one timer at each core.
class TimerManager {
 public:
  static TimerManager& GetTimerManager() {
    static TimerManager m;
    return m;
  }

  static Timer& GetCurrentTimer() { return GetTimerManager().GetTimer(); }

  Timer& GetTimer();

  // Install PIC based timer. Must be installed when running at single core.
  // After booting other cores, we must disable and start using APIC timers.
  void InstallPICTimer();

  // MUST be called at BSP once.
  void InstallAPICTimer(int num_cores);

  void StartAPICTimer();
  void RegisterAlarmClock();

  void Calibrate();

 private:
  TimerManager();

  std::vector<Timer> timers_;
};

}  // namespace Kernel

// This function will be called in assembly based timer interrupt handler.
extern "C" void TimerCaller(Kernel::CPUInterruptHandlerArgs* args,
                            Kernel::InterruptHandlerSavedRegs* regs);
#endif
