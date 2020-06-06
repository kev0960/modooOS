#include "kthread.h"

#include "../std/printf.h"
#include "cpu.h"
#include "kernel_context.h"
#include "kernel_util.h"
#include "kmalloc.h"
#include "qemu_log.h"
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

bool KernelThread::kInitThreadDone = false;

KernelThread* KernelThread::CurrentThread() {
  KernelThread* current_thread;

  // Current thread block is stored in gs register.
  // Note that segment register does not support lea. That means,
  // mov %gs:0 %eax is equivalent to mov [%gs:0] %eax. Thus, we have to store
  // the ADDRESS of the memory that contains the address of the thread block.
  // This is why we are passing self.
  asm volatile("movq %%fs:0, %0" : "=r"(current_thread)::);
  return current_thread;
}

void KernelThread::InitThread() {
  KernelThread* init_thread = new KernelThread(nullptr, false);
  SetCurrentThread(init_thread);

  kInitThreadDone = true;
}

void KernelThread::SetCurrentThread(KernelThread* thread) {
  auto current_thread_addr = reinterpret_cast<uint64_t>(&thread->self);
  CPURegsAccessProvider::SetMSR(kFSBaseMSR, current_thread_addr,
                                current_thread_addr >> 32);
}

KernelThread::KernelThread(EntryFuncType entry_function, bool need_stack,
                           bool in_same_cpu_id)
    : self(this),
      status_(THREAD_RUN),
      kernel_list_elem_(&KernelThreadScheduler::GetKernelThreadList()),
      in_queue_(false),
      in_same_cpu_id_(in_same_cpu_id) {
  thread_id_ = ThreadIdManager::GetThreadId();

  // The rip of this function will be entry_function.
  kernel_regs_.rip = reinterpret_cast<uint64_t>(entry_function);

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

  // Set it as interrupt enabled RFLAGS.
  kernel_regs_.rflags = 0x200;
  kernel_stack_top_ = kernel_regs_.rsp;

  kernel_list_elem_.Set(this);
  InitSavedRegs(&kernel_regs_.regs);

  ASSERT(KERNEL_THREAD_SAVED_KERNEL_TOP_OFFSET ==
         OffsetOf(*this, &KernelThread::kernel_stack_top_));

  cpu_id_ = CPUContextManager::GetCurrentCPUId();
}

void KernelThread::Start() {
  if (in_same_cpu_id_) {
    KernelThreadScheduler::GetKernelThreadScheduler().EnqueueThread(
        &kernel_list_elem_);
  } else {
    KernelThreadScheduler::GetKernelThreadScheduler().EnqueueThreadFirstTime(
        &kernel_list_elem_);
  }
}

void KernelThread::Terminate() {
  // No need to remove thread from the queue since running thread is not in the
  // queue.

  status_ = THREAD_TERMINATE;
  KernelThreadScheduler::GetKernelThreadScheduler().Yield();
  kprintf("[terminate] %d \n", thread_id_);
  PANIC();
}

void KernelThread::TerminateInInterruptHandler(
    CPUInterruptHandlerArgs* args, InterruptHandlerSavedRegs* regs) {
  // No need to remove thread from the queue since running thread is not in the
  // queue.

  status_ = THREAD_TERMINATE;
  KernelThreadScheduler::GetKernelThreadScheduler().YieldInInterruptHandler(
      args, regs);
  kprintf("[terminate in intr] %d \n", thread_id_);
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

KernelThread::~KernelThread() {
  QemuSerialLog::Logf("Destroy thread\n");
  // Previously, kernel_stack_top_ = stack[kKernelThreadStackSize / 8 - 1]
  uint64_t stack_bottom =
      kernel_stack_top_ - sizeof(uint64_t) * (kKernelThreadStackSize / 8 - 1);
  kfree((void*)(stack_bottom));
}

}  // namespace Kernel
