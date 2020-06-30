#include "semaphore.h"

#include "scheduler.h"

namespace Kernel {

void Semaphore::Up() {
  bool need_irq_lock = CPURegsAccessProvider::IsInterruptEnabled();

  IrqLock lock;
  if (need_irq_lock) {
    lock.lock();
  }

  cnt_ += 1;
  if (!waiters_.empty()) {
    // Get the first thread in the waiting queue.
    KernelListElement<KernelThread*>* elem = waiters_.pop_front();

    /*
    if (elem->Get()->CpuId() != CPUContextManager::GetCurrentCPUId()) {
      QemuSerialLog::Logf("Thread cpu : %d vs Current : %d \n",
                          elem->Get()->CpuId(),
                          CPUContextManager::GetCurrentCPUId());
      PrintStackTrace();
    }*/

    // Mark it as a runnable thread.
    elem->Get()->WakeUp();

    // Put back it into the scheuduling queue.
    KernelThreadScheduler::GetKernelThreadScheduler().EnqueueThread(elem);
  }

  if (need_irq_lock) {
    lock.unlock();
  }
}

void Semaphore::Down() {
  bool need_irq_lock = CPURegsAccessProvider::IsInterruptEnabled();
  IrqLock lock;

  while (true) {
    if (need_irq_lock) {
      lock.lock();
    }

    if (cnt_ > 0) {
      cnt_--;
      if (need_irq_lock) {
        lock.unlock();
      }
      return;
    }

    // If there is something to switch into, then we make current thread
    // sleep. Otherwise, do nothing.
    if (KernelThreadScheduler::GetKernelThreadList().size() > 0) {
      // If not able to acquire semaphore, put itself into the waiter queue.
      // First mark it as non runnable.
      auto* current = KernelThread::CurrentThread();
      current->MakeSleep();

      // Then move it to the waiters_ list.
      current->GetKenrelListElem()->ChangeList(&waiters_);
      current->GetKenrelListElem()->PushBack();
    }

    KernelThreadScheduler::GetKernelThreadScheduler().Yield();

    if (need_irq_lock) {
      lock.unlock();
    }
  }
}

}  // namespace Kernel
