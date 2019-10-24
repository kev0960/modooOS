#ifndef SCHEDULER_H
#define SCHEDULER_H

#include "interrupt.h"
#include "kthread.h"
#include "list.h"

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

  void YieldInInterruptHandler(CPUInterruptHandlerArgs* args);

 private:
  KernelThreadScheduler();
  KernelListElement<KernelThread*>* PopNextThreadToRun();

  KernelList<KernelThread*> kernel_thread_list_;
};

}  // namespace Kernel
#endif

