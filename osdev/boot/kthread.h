#ifndef KTHREAD_H
#define KTHREAD_H

#include "list.h"

namespace Kernel {

struct SavedRegisters {
  void* rip;
  void* rsp;
  void* rflags;
} __attribute__((packed));

class KernelThread {
  enum ThreadStatus { THREAD_RUN, THREAD_SLEEP, THREAD_TERMINATE };

 public:
  KernelThread();

  static KernelThread* CurrentThread();
  static void InitThread();

  SavedRegisters* GetSavedRegs() { return &regs_; }

 private:
  size_t thread_id;
  SavedRegisters regs_;

  ThreadStatus status_;
  KernelListElement<KernelThread*> kernel_list_elem;
};

class Semaphore {
 public:
  Semaphore(int cnt) : cnt_(cnt) {}

  void Up();
  void Down();

 private:
  int cnt_ = 0;
  KernelList<KernelThread*> waiters_;
};

}  // namespace Kernel

#endif
