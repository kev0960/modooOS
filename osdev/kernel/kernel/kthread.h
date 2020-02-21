#ifndef KTHREAD_H
#define KTHREAD_H

#include "interrupt.h"
#include "kernel_list.h"

namespace Kernel {

struct SavedRegisters {
  uint64_t rip;
  uint64_t rsp;
  uint64_t rflags;
  uint64_t cs;
  uint64_t ss;

  InterruptHandlerSavedRegs regs;
} __attribute__((packed));

class KernelThread {
  public:
  enum ThreadStatus { THREAD_RUN, THREAD_SLEEP, THREAD_TERMINATE };

 public:
  using EntryFuncType = void (*)();

  // If need_stack is true, then rsp will be used as a RSP.
  KernelThread(EntryFuncType entry_function, bool need_stack = true);

  static KernelThread* CurrentThread();
  static void SetCurrentThread(KernelThread* thread);
  static void InitThread();

  // Called when the thread is done executing.
  static void Done() { CurrentThread()->Terminate(); }

  SavedRegisters* GetSavedKernelRegs() { return &kernel_regs_; }
  size_t Id() const { return thread_id_; }
  KernelListElement<KernelThread*>* GetKenrelListElem() {
    return &kernel_list_elem_;
  }
  bool IsRunnable() const { return status_ == THREAD_RUN; }

  // Starts the thread by adding to the scheduling queue.
  void Start();

  // Terminates the thread by removing from the scheduling queue.
  void Terminate();
  void TerminateInInterruptHandler(CPUInterruptHandlerArgs* args,
                                   InterruptHandlerSavedRegs* regs);

  // Wait until this thread finishes.
  void Join();

  void MakeSleep() { status_ = THREAD_SLEEP; }
  void MakeRun() { status_ = THREAD_RUN; }

  virtual bool IsKernelThread() const { return true; }
  virtual bool IsInKernelSpace() const { return true; }

  // Should not be called for kernel thread.
  virtual uint64_t* GetPageTableBaseAddress() const;

  // The pointer that stores the current thread info. We cannot pass the this
  // pointer since this does not take any memory space.
  KernelThread* const self;

  size_t lock_wait_cnt;
  uint64_t saved_rbp;

  // This is used when the kernel thread exits the page fault handler.
  uint64_t empty_kernel_stack_;

  ThreadStatus status_;
 protected:
  size_t thread_id_;
  SavedRegisters kernel_regs_;

  KernelListElement<KernelThread*> kernel_list_elem_;
};

class Semaphore {
 public:
  Semaphore(int cnt) : cnt_(cnt) {}

  // If we are using semaphore inside of the interrupt handler, then we should
  // set without_lock as true.
  void Up();
  void Down();
  void DownInInterruptHandler(CPUInterruptHandlerArgs* args,
                              InterruptHandlerSavedRegs* regs);

 private:
  int cnt_;
  KernelList<KernelThread*> waiters_;
};

}  // namespace Kernel

#endif
