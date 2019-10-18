#include "kthread.h"

namespace Kernel {

KernelList<KernelThread*> kernel_thread_list;

KernelThread* KernelThread::CurrentThread() {
  KernelThread* current_thread;

  // Current thread block is stored in fs register.
  asm volatile("movq %%fs, %0" : "=r"(current_thread)::);
  return current_thread;
}

void KernelThread::InitThread() {
  KernelThread* init_thread = new KernelThread();

  // Current running thread becomes the init thread.
  asm volatile("movq %0, %%fs" ::"r"(init_thread) :);
}

KernelThread::KernelThread()
    : status_(THREAD_RUN), kernel_list_elem(&kernel_thread_list) {
  thread_id = ThreadIdManager::GetThreadId();

  // Put the thread into the thread list.
  kernel_list_elem.Set(this);
  kernel_list_elem.PushBack();
}

}  // namespace Kernel
