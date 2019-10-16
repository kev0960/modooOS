#ifndef KTHREAD_H
#define KTHREAD_H

namespace Kernel {

class KernelThread {
  enum ThreadStatus { THREAD_RUN, THREAD_SLEEP, THREAD_TERMINATE };

 public:
  KernelThread();

 private:
  ThreadStatus status_;
};

}  // namespace Kernel

#endif
