#ifndef KTHREAD_H
#define KTHREAD_H

#include "list.h"

namespace Kernel {
namespace {

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
    // TODO add lock guard.
    return current_id++;
  }

  size_t current_id = 0;
};

}  // namespace

class KernelThread {
  enum ThreadStatus { THREAD_RUN, THREAD_SLEEP, THREAD_TERMINATE };

 public:
  KernelThread();

  static KernelThread* CurrentThread();
  static void InitThread();

 private:
  size_t thread_id;

  ThreadStatus status_;
  KernelListElement<KernelThread*> kernel_list_elem;

  // RSP that is saved right before context switch.
  void* saved_rsp;
};

// List of entire kernel threads.
extern KernelList<KernelThread*> kernel_thread_list;

}  // namespace Kernel

#endif
