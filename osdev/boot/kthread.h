#ifndef KTHREAD_H
#define KTHREAD_H

#include "interrupt.h"
#include "list.h"

namespace Kernel {

struct SavedRegisters {
  uint64_t rip;
  uint64_t rsp;
  uint64_t rflags;
  InterruptHandlerSavedRegs regs;
} __attribute__((packed));

class KernelThread {
  enum ThreadStatus { THREAD_RUN, THREAD_SLEEP, THREAD_TERMINATE };

 public:
  using EntryFuncType = void (*)();

  KernelThread(EntryFuncType entry_function, bool need_stack = true);

  static KernelThread* CurrentThread();
  static void SetCurrentThread(KernelThread* thread);
  static void InitThread();

  // Called when the thread is done executing.
  static void Done() { CurrentThread()->Terminate(); }

  SavedRegisters* GetSavedRegs() { return &regs_; }
  size_t Id() const { return thread_id_; }
  KernelListElement<KernelThread*>* GetKenrelListElem() {
    return &kernel_list_elem_;
  }
  bool IsRunnable() const { return status_ == THREAD_RUN; }

  // Starts the thread by adding to the scheduling queue.
  void Start();

  // Terminates the thread by removing from the scheduling queue.
  void Terminate();

  // Wait until this thread finishes.
  void Join();

  void MakeSleep() { status_ = THREAD_SLEEP; }
  void MakeRun() { status_ = THREAD_RUN; }

  // The pointer that stores the current thread info. We cannot pass the this
  // pointer since this does not take any memory space.
  KernelThread* const self;

  size_t lock_wait_cnt;
  uint64_t saved_rbp;

 private:
  size_t thread_id_;
  SavedRegisters regs_;

  ThreadStatus status_;
  KernelListElement<KernelThread*> kernel_list_elem_;
};

class Semaphore {
 public:
  Semaphore(int cnt) : cnt_(cnt) {}

  // If we are using semaphore inside of the interrupt handler, then we should
  // set without_lock as true.
  void Up(bool inside_interrupt_handler = false);
  void Down(bool inside_interrupt_handler = false);

 private:
  int cnt_;
  KernelList<KernelThread*> waiters_;
};

}  // namespace Kernel

#endif
