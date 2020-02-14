#include "kthread.h"
#include "../std/printf.h"
#include "cpu.h"
#include "kernel_util.h"
#include "kmalloc.h"
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
  kernel_regs_.rip = reinterpret_cast<uint64_t>(entry_function);

  kprintf("rip : %lx \n", kernel_regs_.rip);
  kernel_regs_.cs = 0x8;
  kernel_regs_.ss = 0x10;

  // Set up the thread's stack.
  if (need_stack) {
    uint64_t* stack =
        reinterpret_cast<uint64_t*>(kmalloc(kKernelThreadStackSize));

    constexpr int num_stack_elements = kKernelThreadStackSize / 8;
    // Since the stack is the highest address, set rsp as the top most address
    // of the stack.
    kernel_regs_.rsp =
        reinterpret_cast<uint64_t>(&stack[num_stack_elements - 1]);

    // We are pushing Done as a return address. When the thread finishes, it
    // will return to Done() which will finish the thread.
    stack[num_stack_elements - 1] = reinterpret_cast<uint64_t>(&Done);
  }

  // Set it as current RFLAGS.
  // TODO Think more carefully about this.
  kernel_regs_.rflags = GetRFlags();

  kernel_list_elem_.Set(this);
  InitSavedRegs(&kernel_regs_.regs);
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
  kprintf("TErminate?? %d \n", thread_id_);
  KernelThreadScheduler::GetKernelThreadScheduler().Yield();
  kprintf("Reached here? %d \n", thread_id_);
  PANIC();
}

void KernelThread::TerminateInInterruptHandler(
    CPUInterruptHandlerArgs* args, InterruptHandlerSavedRegs* regs) {
  // No need to remove thread from the queue since running thread is not in the
  // queue.

  status_ = THREAD_TERMINATE;
  kprintf("TErminate?? %d \n", thread_id_);
  KernelThreadScheduler::GetKernelThreadScheduler().YieldInInterruptHandler(
      args, regs);
  kprintf("Reached here? %d \n", thread_id_);
  PANIC();
}

// Do not optimze this function. When optimizing turned on, the compiler will
void __attribute__((optimize("O0"))) KernelThread::Join() {
  while (status_ != THREAD_TERMINATE) {
    KernelThreadScheduler::GetKernelThreadScheduler().Yield();
  }
}

uint64_t* KernelThread::GetPageTableBaseAddress() const {
  kprintf(
      "Should not try to get page table base address of the kernel thread. "
      "Why do you even need when we don't need to change CR3 for context "
      "switch?\n");
  PANIC();
  return nullptr;
}

void Semaphore::Up(bool inside_interrupt_handler) {
  IrqLock lock;
  if (!inside_interrupt_handler) {
    lock.lock();
  }

  cnt_ += 1;
  if (!waiters_.empty()) {
    // Get the first thread in the waiting queue.
    KernelListElement<KernelThread*>* elem = waiters_.pop_front();

    // Mark it as a runnable thread.
    elem->Get()->MakeRun();

    // Put back it into the scheuduling queue.
    elem->ChangeList(&KernelThreadScheduler::GetKernelThreadList());
    elem->PushBack();
  }

  if (!inside_interrupt_handler) {
    lock.unlock();
  }
}

void Semaphore::Down(bool inside_interrupt_handler) {
  IrqLock lock;

  while (true) {
    if (!inside_interrupt_handler) {
      lock.lock();
    }

    if (cnt_ > 0) {
      cnt_--;
      if (!inside_interrupt_handler) {
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

    if (!inside_interrupt_handler) {
      lock.unlock();
    }
  }
}

}  // namespace Kernel
