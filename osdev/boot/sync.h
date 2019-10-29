#ifndef SYNC_H
#define SYNC_H

#include "cpu.h"
#include "kthread.h"
#include "printf.h"
#include "types.h"

namespace Kernel {
namespace std {

template <typename Mutex>
class lock_guard {
 public:
  lock_guard(Mutex& m) : m_(m) { m_.lock(); }

  ~lock_guard() { m_.unlock(); }

 private:
  Mutex& m_;
};

}  // namespace std

class Lock {
 public:
  virtual void lock() = 0;
  virtual void unlock() = 0;
  virtual bool try_lock() = 0;
};

// IRQ based lock. It disables the entire interrupt so that it can run
// exclusively.
class IrqLock : public Lock {
  // Interrupt flag position.
  const static uint64_t INT_FLAG = 0x200;

 public:
  void lock() override {
    // Disable current interrupt.
    saved_rflags_ = GetRFlags();

    // Note that there can be a context switch in between here that might change
    // the saved rflags. But I hope it is not a big deal anyway.
    DisableInterrupt();
  }

  void unlock() override {
    SetRFlags(saved_rflags_);

    EnableInterrupt();
  }

  // Do not use
  bool try_lock() override { return false; }

 private:
  uint64_t saved_rflags_;
};

// Simple spin lock. It spins until it can acquire.
class SpinLock : public Lock {
 public:
  void lock() override {
    while (__atomic_test_and_set(&acquired, __ATOMIC_ACQUIRE)) {
      KernelThread::CurrentThread()->lock_wait_cnt++;
    }
    lock_holder_ = KernelThread::CurrentThread();
  }

  void unlock() override {
    lock_holder_ = nullptr;
    __atomic_clear(&acquired, __ATOMIC_RELEASE);
  }

  bool try_lock() override {
    bool expected = false;
    return __atomic_compare_exchange_n(&acquired, &expected, true, false,
                                       __ATOMIC_ACQUIRE, __ATOMIC_ACQUIRE);
  }

  KernelThread* lock_holder_;

 private:
  bool acquired = false;
};

}  // namespace Kernel

#endif
