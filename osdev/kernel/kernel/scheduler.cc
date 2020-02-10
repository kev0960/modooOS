#include "scheduler.h"
#include "cpu.h"
#include "printf.h"
#include "timer.h"
#include "vga_output.h"

namespace Kernel {

namespace {

template <typename T, typename U>
void CopyCPUInteruptHandlerArgs(T* to, U* from) {
  to->rip = from->rip;
  to->rsp = from->rsp;
  to->rflags = from->rflags;
}

}  // namespace

KernelListElement<KernelThread*>* KernelThreadScheduler::PopNextThreadToRun() {
  if (!kernel_thread_list_.size()) {
    return nullptr;
  }

  // Pop the current thread and run.
  while (kernel_thread_list_.size()) {
    KernelListElement<KernelThread*>* next_thread_element =
        kernel_thread_list_.pop_front();
    if (next_thread_element->Get()->IsRunnable()) {
      return next_thread_element;
    }
  }

  // Unreachable.
  return nullptr;
}

// static int cnt = 0;
void KernelThreadScheduler::YieldInInterruptHandler(
    CPUInterruptHandlerArgs* args, InterruptHandlerSavedRegs* regs) {
  auto* next_thread_element = PopNextThreadToRun();
  if (next_thread_element == nullptr) {
    return;
  }

  KernelThread* current_thread = KernelThread::CurrentThread();
  if (current_thread->IsRunnable()) {
    // Move the current thread to run at the back of the queue.
    kernel_thread_list_.push_back(current_thread->GetKenrelListElem());
  }

  auto* current_thread_regs = current_thread->GetSavedRegs();
  CopyCPUInteruptHandlerArgs(current_thread_regs, args);
  current_thread_regs->regs = *regs;

  // Now we have to change interrupt frame to the target threads' return info.
  KernelThread* next_thread = next_thread_element->Get();
  auto* next_thread_regs = next_thread->GetSavedRegs();
  CopyCPUInteruptHandlerArgs(args, next_thread_regs);
  *regs = next_thread_regs->regs;

  // If the next thread is a user process, then we have to reset CR3
  if (!next_thread->IsKernelThread()) {
    CPURegsAccessProvider::SetCR3(
        reinterpret_cast<uint64_t>(next_thread->GetPageTableBaseAddress()));
  }

  KernelThread::SetCurrentThread(next_thread);
  // Since we changed the interrupt handler's stack to the next thread's
  // stack, the handler will return where the next thread has switched.
}

void KernelThreadScheduler::Yield() { asm volatile("int $0x30\n"); }

}  // namespace Kernel

extern "C" void YieldInInterruptHandlerCaller(
    Kernel::CPUInterruptHandlerArgs* args,
    Kernel::InterruptHandlerSavedRegs* regs) {
  Kernel::KernelThreadScheduler::GetKernelThreadScheduler()
      .YieldInInterruptHandler(args, regs);
}
