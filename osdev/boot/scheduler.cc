#include "scheduler.h"
#include "timer.h"

namespace Kernel {

KernelListElement<KernelThread*>* KernelThreadScheduler::PopNextThreadToRun() {
  if (kernel_thread_list_.size() <= 1) {
    return nullptr;
  }

  // Pop the current thread and run.
  KernelListElement<KernelThread*>* next_thread_element =
      kernel_thread_list_.pop_front();
  return next_thread_element;
}

void KernelThreadScheduler::YieldInInterruptHandler(
    CPUInterruptHandlerArgs* args) {
  auto* next_thread_element = PopNextThreadToRun();
  if (next_thread_element == nullptr) {
    return;
  }

  // Move the next thread to run at the back of the queue.
  kernel_thread_list_.push_back(next_thread_element);

  KernelThread* current_thread = KernelThread::CurrentThread();

  auto* current_thread_regs = current_thread->GetSavedRegs();
  current_thread_regs->rip = reinterpret_cast<void*>(args->rip);
  current_thread_regs->rsp= reinterpret_cast<void*>(args->rsp);
  current_thread_regs->rflags= reinterpret_cast<void*>(args->rflags);

  KernelThread* next_thread = next_thread_element->Get();
}
}  // namespace Kernel
