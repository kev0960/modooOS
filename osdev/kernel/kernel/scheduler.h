#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "interrupt.h"
#include "kthread.h"
#include "../std/list.h"

namespace Kernel {

// Simple kernel thread scheduler.
class KernelThreadScheduler {
 public:
  static KernelThreadScheduler& GetKernelThreadScheduler() {
    static KernelThreadScheduler scheduler;
    return scheduler;
  }

  static KernelList<KernelThread*>& GetKernelThreadList() {
    return GetKernelThreadScheduler().kernel_thread_list_;
  }

  KernelThreadScheduler(const KernelThreadScheduler&) = delete;
  void operator=(const KernelThreadScheduler&) = delete;

  void YieldInInterruptHandler(CPUInterruptHandlerArgs* args,
                               InterruptHandlerSavedRegs* regs);
  void Yield();

 private:
  KernelThreadScheduler() = default;
  KernelListElement<KernelThread*>* PopNextThreadToRun();

  // Scheduling queue.
  // NOTE that currently running thread (on CPU) is NOT on the queue.
  KernelList<KernelThread*> kernel_thread_list_;
};

extern "C" void YieldInInterruptHandlerCaller(
    Kernel::CPUInterruptHandlerArgs* args,
    Kernel::InterruptHandlerSavedRegs* regs);
}  // namespace Kernel
#endif

