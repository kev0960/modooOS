#ifndef SEMAPHORE_H
#define SEMAPHORE_H

#include "kthread.h"

namespace Kernel {

class Semaphore {
 public:
  Semaphore(int cnt) : cnt_(cnt) {}

  // Semaphore is not copiable.
  Semaphore(const Semaphore&) = delete;

  Semaphore(Semaphore&& sema) : waiters_(std::move(sema.waiters_)) {
    cnt_ = sema.cnt_;
  }

  // If we are using semaphore inside of the interrupt handler, then we should
  // set without_lock as true.
  void Up();
  void Down();

 private:
  int cnt_;
  KernelList<KernelThread*> waiters_;
};

}  // namespace Kernel

#endif
