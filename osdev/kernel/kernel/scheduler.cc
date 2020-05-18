#include "scheduler.h"

#include "../std/printf.h"
#include "cpu.h"
#include "descriptor_table.h"
#include "process.h"
#include "qemu_log.h"
#include "timer.h"
#include "vga_output.h"

namespace Kernel {

namespace {

template <typename T, typename U>
void CopyCPUInteruptHandlerArgs(T* to, U* from) {
  to->rip = from->rip;
  to->rsp = from->rsp;
  to->rflags = from->rflags;
  to->ss = from->ss;
  to->cs = from->cs;
}

}  // namespace

KernelList<KernelThread*>& KernelThreadScheduler::GetKernelThreadList() {
  return GetKernelThreadScheduler()
      .kernel_thread_list_[CPUContextManager::GetCurrentCPUId()];
}

KernelListElement<KernelThread*>* KernelThreadScheduler::PopNextThreadToRun() {
  if (kernel_thread_list_.size() == 0) {
    return nullptr;
  }

  if (!GetKernelThreadList().size()) {
    return nullptr;
  }

  // Pop the current thread and run.
  while (GetKernelThreadList().size()) {
    /*
    QemuSerialLog::Logf(
        "[bef]h %lx t %lx sz : %d %d\n", GetKernelThreadList().head_,
        GetKernelThreadList().tail_, GetKernelThreadList().size_,
        CPURegsAccessProvider::IsInterruptEnabled());
        */
    KernelListElement<KernelThread*>* next_thread_element =
        GetKernelThreadList().pop_front();
    /*
    QemuSerialLog::Logf(
        "[aft]h %lx t %lx sz : %d intr : %d\n", GetKernelThreadList().head_,
        GetKernelThreadList().tail_, GetKernelThreadList().size_,
        CPURegsAccessProvider::IsInterruptEnabled());*/
    if (next_thread_element->Get()->IsRunnable()) {
      return next_thread_element;
    }
  }

  // Unreachable.
  return nullptr;
}

void KernelThreadScheduler::YieldInInterruptHandler(
    CPUInterruptHandlerArgs* args, InterruptHandlerSavedRegs* regs) {
  // Scheduling is not enabled yet!
  if (queue_locks_.empty() || kernel_thread_list_.empty()) {
    return;
  }

  queue_locks_[CPUContextManager::GetCurrentCPUId()].lock();

  auto* next_thread_element = PopNextThreadToRun();
  if (next_thread_element == nullptr) {
    queue_locks_[CPUContextManager::GetCurrentCPUId()].unlock();
    return;
  }

  KernelThread* current_thread = KernelThread::CurrentThread();
  ASSERT(next_thread_element->Get()->CpuId() ==
         CPUContextManager::GetCurrentCPUId());
  // Only push when the current thread is not already in queue.
  // This is because the race condtion that can happen between the scheduler and
  // the semaphore. Suppose thread A calls Down() on Semaphore S.
  // 1) A goes in to the waiting queue. A is now marked as sleep. A yields.
  // 2) Other thread in a different core calls S.Up(). A is now marked as
  // RUNNING and gets enqueued into the scheduling queue.
  // 3) Since A has yielded   on (1), scheduler checks whether the current
  // thread is RUNNING. Since it was marked as RUNNING, this will be put into
  // the queue. Hence two same threads are put into the queue.
  //
  // This problem can be prevented by checking whether the thread is already in
  // queue on step (3).
  if (current_thread->IsRunnable() && !current_thread->IsInQueue()) {
    /*
    QemuSerialLog::Logf("[CPU %d]Thread cpu id : %d \n",
                        CPUContextManager::GetCurrentCPUId(),
                        current_thread->CpuId());
                        */
    // Move the current thread to run at the back of the queue.
    GetKernelThreadList().push_back(current_thread->GetKenrelListElem());
    current_thread->SetInQueue(true);
  } else {
    // This thread is no longer in queue (since it is sleeping).
    current_thread->SetInQueue(false);
  }
  /*
  QemuSerialLog::Logf("[aft enq]h %lx t %lx sz : %d\n",
                      GetKernelThreadList().head_, GetKernelThreadList().tail_,
                      GetKernelThreadList().size_);
  */

  SavedRegisters* current_thread_regs = nullptr;
  if (args->cs == kKernelCodeSegment) {
    current_thread_regs = current_thread->GetSavedKernelRegs();
  } else {
    current_thread_regs =
        static_cast<Process*>(current_thread)->GetSavedUserRegs();
  }
  CopyCPUInteruptHandlerArgs(current_thread_regs, args);
  current_thread_regs->regs = *regs;

  // Now we have to change interrupt frame to the target threads' return info.
  KernelThread* next_thread = next_thread_element->Get();
  next_thread->SetInQueue(false);

  /*
  int cpu_id = CPUContextManager::GetCurrentCPUId();
  if (cpu_id >= 0) {
    QemuSerialLog::Logf("[CPU %d] Schedule!(%d) -> (%d) %lx \n",
                        CPUContextManager::GetCurrentCPUId(),
                        current_thread->Id(), next_thread->Id(),
                        CPUContextManager::GetCurrentCPUId());
  }
  if (CPUContextManager::GetCurrentCPUId() == 1) {
    sp.lock();
  kprintf("Schedule!(%d) -> (%d) %lx \n", current_thread->Id(),
          next_thread->Id(), CPUContextManager::GetCurrentCPUId());
  sp.unlock();
  }
      current_thread->GetSavedKernelRegs()->rflags,
      current_thread->GetKernelStackTop());
      */
  // If the target thread is a user process (i.e we are jumping into the user
  // space), we have to set the interrupt frame's RSP as User's RSP (user_rsp)
  // instead of the kernel rsp.
  SavedRegisters* next_thread_regs;
  if (next_thread->IsInKernelSpace()) {
    next_thread_regs = next_thread->GetSavedKernelRegs();
    CopyCPUInteruptHandlerArgs(args, next_thread_regs);
  } else {
    Process* process = static_cast<Process*>(next_thread);
    next_thread_regs = process->GetSavedUserRegs();
    CopyCPUInteruptHandlerArgs(args, next_thread_regs);

    // Also set TSS RSP0 as current user process's kernel stack rsp.
    TaskStateSegmentManager::GetTaskStateSegmentManager().SetRSP0(
        next_thread->GetSavedKernelRegs()->rsp);
  }
  *regs = next_thread_regs->regs;

  // If the next thread is a user process, then we have to reset CR3
  if (!next_thread->IsKernelThread()) {
    CPURegsAccessProvider::SetCR3(
        reinterpret_cast<uint64_t>(next_thread->GetPageTableBaseAddress()));
  }

  KernelThread::SetCurrentThread(next_thread);
  // Since we changed the interrupt handler's stack to the next thread's
  // stack, the handler will return where the next thread has switched.

  queue_locks_[CPUContextManager::GetCurrentCPUId()].unlock();
}

void KernelThreadScheduler::Yield() { asm volatile("int $0x30\n"); }

void KernelThreadScheduler::EnqueueThread(
    KernelListElement<KernelThread*>* elem) {
  uint32_t cpu_id = elem->Get()->CpuId();

  // QemuSerialLog::Logf("Enqueue %d --> %d \n", elem->Get()->Id(), cpu_id);
  // Acquire lock on the scheduling queue.
  queue_locks_[cpu_id].lock();

  // Enqueue thread.
  elem->ChangeList(&kernel_thread_list_[cpu_id]);
  /*
  QemuSerialLog::Logf("[EnqueueThread]h %lx t %lx sz : %d t:%d cpu %d t:%d\n",
                      kernel_thread_list_[cpu_id].head_,
                      kernel_thread_list_[cpu_id].tail_,
                      kernel_thread_list_[cpu_id].size_, elem->Get()->Id(),
                      cpu_id, elem->Get()->Id());
                      */

  // TODO Figure out when elem->Get()->IsInQueue() can be true.
  // (Happens rarely but can't figure out why it is happening).
  if (!elem->Get()->IsInQueue()) {
    elem->PushBack();
    elem->Get()->SetInQueue(true);
  }
  /*
  QemuSerialLog::Logf("[aftEnqueueThread]h %lx t %lx sz : %d\n",
                      kernel_thread_list_[cpu_id].head_,
                      kernel_thread_list_[cpu_id].tail_,
                      kernel_thread_list_[cpu_id].size_);
                      */

  queue_locks_[cpu_id].unlock();
}

void KernelThreadScheduler::SetCoreCount(int num_core) {
  kernel_thread_list_.reserve(num_core);
  queue_locks_.reserve(num_core);

  for (int i = 0; i < num_core; i++) {
    kernel_thread_list_.push_back(KernelList<KernelThread*>());
    queue_locks_.push_back(MultiCoreSpinLock());
  }
}

}  // namespace Kernel

extern "C" void YieldInInterruptHandlerCaller(
    Kernel::CPUInterruptHandlerArgs* args,
    Kernel::InterruptHandlerSavedRegs* regs) {
  Kernel::KernelThreadScheduler::GetKernelThreadScheduler()
      .YieldInInterruptHandler(args, regs);
}
