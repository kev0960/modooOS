#include "kthread.h"
#include "kmalloc.h"
#include "printf.h"
#include "scheduler.h"
#include "sync.h"

namespace Kernel {
namespace {

constexpr int kKernelThreadStackSize = 8192;  // 8 KB
constexpr int kFSBaseMSR = 0xC0000100;

void InitSavedRegs(InterruptHandlerSavedRegs* saved_regs) {
  saved_regs->rbp = 0;
  saved_regs->rax = 0;
  saved_regs->rbx = 0;
  saved_regs->rcx = 0;
  saved_regs->rdx = 0;
  saved_regs->rsi = 0;
  saved_regs->rdi = 0;
  saved_regs->r8 = 0;
  saved_regs->r9 = 0;
  saved_regs->r10 = 0;
  saved_regs->r11 = 0;
  saved_regs->r12 = 0;
  saved_regs->r13 = 0;
  saved_regs->r14 = 0;
  saved_regs->r15 = 0;
}

};  // namespace

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
  // Note that segment register does not support lea. That means,
  // mov %fs:0 %eax is equivalent to mov [%fs:0] %eax. Thus, we have to store
  // the ADDRESS of the memory that contains the address of the thread block.
  // This is why we are passing self.
  asm volatile("movq %%fs:0, %0" : "=r"(current_thread)::);
  return current_thread;
}

void KernelThread::InitThread() {
  KernelThread* init_thread = new KernelThread(nullptr, false);
  SetCurrentThread(init_thread);
}

void KernelThread::SetCurrentThread(KernelThread* thread) {
  auto current_thread_addr = reinterpret_cast<uint64_t>(&thread->self);
  SetMSR(kFSBaseMSR, current_thread_addr, current_thread_addr >> 32);
}

KernelThread::KernelThread(EntryFuncType entry_function, bool need_stack)
    : self(this),
      status_(THREAD_RUN),
      kernel_list_elem_(&KernelThreadScheduler::GetKernelThreadList()) {
  thread_id_ = ThreadIdManager::GetThreadId();

  // The rip of this function will be entry_function.
  regs_.rip = reinterpret_cast<uint64_t>(entry_function);

  // Set up the thread's stack.
  if (need_stack) {
    uint64_t* stack =
        reinterpret_cast<uint64_t*>(kmalloc(kKernelThreadStackSize));

    constexpr int num_stack_elements = kKernelThreadStackSize / 8;
    // Since the stack is the highest address, set rsp as the top most address
    // of the stack.
    regs_.rsp = reinterpret_cast<uint64_t>(&stack[num_stack_elements - 1]);

    // We are pushing Done as a return address. When the thread finishes, it
    // will return to Done() which will finish the thread.
    stack[num_stack_elements - 1] = reinterpret_cast<uint64_t>(&Done);
  }

  // Set it as current RFLAGS.
  // TODO Think more carefully about this.
  regs_.rflags = GetRFlags();

  kernel_list_elem_.Set(this);
  InitSavedRegs(&regs_.regs);
}

void KernelThread::Start() {
  IrqLock m_;

  m_.lock();
  // Put the thread into the scheduling queue.
  kernel_list_elem_.PushBack();

  m_.unlock();
}

void KernelThread::Terminate() {
  // No need to remove thread from the queue since running thread is not in the
  // queue.

  status_ = THREAD_TERMINATE;
  KernelThreadScheduler::GetKernelThreadScheduler().Yield();
  kprintf("Reached here? %d \n", thread_id_);
}

// Do not optimze this function. When optimizing turned on, the compiler will
// not consistently check the value of status_.
void __attribute__((optimize("O0"))) KernelThread::Join() {
  while (status_ != THREAD_TERMINATE) {
    KernelThreadScheduler::GetKernelThreadScheduler().Yield();
  }
}

// static int total = 0;
void Semaphore::Up() {
  IrqLock lock;
  std::lock_guard<IrqLock> lk(lock);

  cnt_ += 1;
  kprintf("up %d (%d) ", cnt_, KernelThread::CurrentThread()->Id());
  if (cnt_ > 1) {
    while (1)
      ;
  }
  /*
  if (total++ < 150) {
    kprintf("up %d (%d) ", cnt_, KernelThread::CurrentThread()->Id());
  }*/
  if (!waiters_.empty()) {
    // Get the first thread in the waiting queue.
    KernelListElement<KernelThread*>* elem = waiters_.pop_front();
    /*
    if (total++ < 150) {
      kprintf("push (%d)->(%d) ", KernelThread::CurrentThread()->Id(),
              elem->Get()->Id());
    }
    */

    kprintf("push (%d)->(%d) ", KernelThread::CurrentThread()->Id(),
            elem->Get()->Id());
    // Mark it as a runnable thread.
    elem->Get()->MakeRun();

    // Put back it into the scheuduling queue.
    elem->ChangeList(&KernelThreadScheduler::GetKernelThreadList());
    elem->PushBack();
  }
}

void __attribute__((optimize("O0"))) Semaphore::Down() {
  IrqLock lock;

  while (true) {
    lock.lock();

    kprintf("down %d (%d) ", cnt_, KernelThread::CurrentThread()->Id());
    if (cnt_ > 0) {
      cnt_--;
      kprintf("exit %d (%d) ", cnt_, KernelThread::CurrentThread()->Id());
      lock.unlock();
      return;
    }

    // If not able to acquire semaphore, put itself into the waiter queue.
    // First mark it as non runnable.
    auto* current = KernelThread::CurrentThread();
    current->MakeSleep();
    /*
    if (total++ < 150) {
      kprintf("sleep %d (%d) ", cnt_, KernelThread::CurrentThread()->Id());
    }
    */

    // Then move it to the waiters_ list.
    current->GetKenrelListElem()->ChangeList(&waiters_);
    current->GetKenrelListElem()->PushBack();

    kprintf("yield %d (%d) ", cnt_, KernelThread::CurrentThread()->Id());
    lock.unlock();

    KernelThreadScheduler::GetKernelThreadScheduler().Yield();
  }
}

}  // namespace Kernel
