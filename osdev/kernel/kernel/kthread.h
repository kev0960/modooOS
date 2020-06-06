#ifndef KTHREAD_H
#define KTHREAD_H

#include "../std/printf.h"
#include "../std/utility.h"
#include "interrupt.h"
#include "kernel_list.h"

#define KERNEL_THREAD_SAVED_KERNEL_TOP_OFFSET 208

namespace Kernel {

struct SavedRegisters {
  uint64_t rip;
  uint64_t rsp;
  uint64_t rflags;
  uint64_t cs;
  uint64_t ss;

  InterruptHandlerSavedRegs regs;
} __attribute__((packed));

using pid_t = size_t;

class KernelThread {
 public:
  enum ThreadStatus {
    THREAD_RUN,
    THREAD_SLEEP,
    THREAD_TERMINATE,

    // If the thread is in terminate ready state, then once the threads leaves
    // the kernel context, then it will never come back to the kernel context.
    // Rather, it will be terminated by the scheduler.
    //
    // This state is added so that when the console kills the running process,
    // we have to make sure that the process only dies when it is in the user
    // context, not in the kernel context because it might be holding some
    // kernel objects.
    THREAD_TERMINATE_READY
  };

 public:
  using EntryFuncType = void (*)();

  static bool kInitThreadDone;

  // If need_stack is true, then rsp will be used as a RSP.
  KernelThread(EntryFuncType entry_function, bool need_stack = true,
               bool in_same_cpu_id = true);

  static KernelThread* CurrentThread();
  static void SetCurrentThread(KernelThread* thread);
  static void InitThread();

  // Called when the thread is done executing.
  static void Done() { CurrentThread()->Terminate(); }

  SavedRegisters* GetSavedKernelRegs() { return &kernel_regs_; }
  pid_t Id() const { return thread_id_; }
  uint32_t CpuId() const { return cpu_id_; }
  void SetCpuId(pid_t id) { cpu_id_ = id; }

  KernelListElement<KernelThread*>* GetKenrelListElem() {
    return &kernel_list_elem_;
  }
  bool IsRunnable() const {
    return (status_ == THREAD_RUN || status_ == THREAD_TERMINATE_READY);
  }

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

  void MakeTerminate() { status_ = THREAD_TERMINATE; }
  void MakeTerminateReady() { status_ = THREAD_TERMINATE_READY; }
  bool IsTerminateReady() const { return status_ == THREAD_TERMINATE_READY; }

  void SetInQueue(bool in_queue) { in_queue_ = in_queue; }
  bool IsInQueue() const { return in_queue_; }

  virtual bool IsKernelThread() const { return true; }
  virtual bool IsInKernelSpace() const { return true; }

  // Should not be called for kernel thread.
  virtual uint64_t* GetPageTableBaseAddress() const;

  uint64_t GetKernelStackTop() const { return kernel_stack_top_; }

  // The pointer that stores the current thread info. We cannot pass the this
  // pointer since this does not take any memory space.
  KernelThread* const self;

  size_t lock_wait_cnt;
  uint64_t saved_rbp;

  ThreadStatus status_;

  virtual ~KernelThread();

 protected:
  pid_t thread_id_;
  SavedRegisters kernel_regs_;

  // This is used when the kernel thread exits the page fault handler.
  uint64_t kernel_stack_top_;

  KernelListElement<KernelThread*> kernel_list_elem_;

  // CPU id where the thread belongs.
  uint32_t cpu_id_;

  bool in_queue_;
  bool in_same_cpu_id_;
};

}  // namespace Kernel

#endif
