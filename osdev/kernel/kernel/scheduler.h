#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "../std/list.h"
#include "../std/vector.h"
#include "cpu_context.h"
#include "interrupt.h"
#include "kthread.h"

namespace Kernel {

// Simple kernel thread scheduler.
class KernelThreadScheduler {
 public:
  static KernelThreadScheduler& GetKernelThreadScheduler() {
    static KernelThreadScheduler scheduler;
    return scheduler;
  }

  static KernelList<KernelThread*>& GetKernelThreadList();

  KernelThreadScheduler(const KernelThreadScheduler&) = delete;
  void operator=(const KernelThreadScheduler&) = delete;

  void YieldInInterruptHandler(CPUInterruptHandlerArgs* args,
                               InterruptHandlerSavedRegs* regs);
  void Yield();

  // Each core will have its own kernel thread list.
  void SetCoreCount(int num_core);

  // Enqueue the kernel thread.
  void EnqueueThread(KernelListElement<KernelThread*>* elem);

 private:
  KernelThreadScheduler() = default;
  KernelListElement<KernelThread*>* PopNextThreadToRun();

  // Scheduling queue.
  // NOTE that currently running thread (on CPU) is NOT on the queue.
  std::vector<KernelList<KernelThread*>> kernel_thread_list_;

  // Locks for the scheduling queue. MUST be obtained when modifying the queue.
  std::vector<MultiCoreSpinLock> queue_locks_;
};

extern "C" void YieldInInterruptHandlerCaller(
    Kernel::CPUInterruptHandlerArgs* args,
    Kernel::InterruptHandlerSavedRegs* regs);
}  // namespace Kernel
#endif

