#include "kthread.h"
#include "sync.h"

namespace Kernel {

class ThreadIdManager {
 public:
  static size_t GetThreadId() {
    static ThreadIdManager thread_id_manager;
    return thread_id_manager.GetId();
  }

  ThreadIdManager(const ThreadIdManager&) = delete;
  void operator=(const ThreadIdManager&) = delete;

 private:
  ThreadIdManager() = default;
  size_t GetId() {
    std::lock_guard<IrqLock> l(lock_);
    return current_id_++;
  }

  size_t current_id_ = 0;
  IrqLock lock_;
};

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

void Semaphore::Up() {}

void Semaphore::Down() {}

}  // namespace Kernel
