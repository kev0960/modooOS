#ifndef SYNC_H
#define SYNC_H

#include "../std/types.h"
#include "cpu.h"
#include "kthread.h"
#include "printf.h"

namespace Kernel {
namespace std {

template <typename M>
class lock_guard {
 public:
  lock_guard(M& m) : m_(m) { m_.lock(); }

  ~lock_guard() { m_.unlock(); }

 private:
  M& m_;
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
  uint64_t saved_rflags_ = 0;
};

// Simple spin lock. It spins until it can acquire.
class SpinLock : public Lock {
 public:
  void lock() override {
    while (__atomic_test_and_set(&acquired, __ATOMIC_ACQUIRE)) {
    }
  }

  void unlock() override { __atomic_clear(&acquired, __ATOMIC_RELEASE); }

  bool try_lock() override {
    bool expected = false;
    return __atomic_compare_exchange_n(&acquired, &expected, true, false,
                                       __ATOMIC_ACQUIRE, __ATOMIC_ACQUIRE);
  }

 private:
  bool acquired = false;
};

// This spinlock can be used even when the spinlock needs to be acuqired inside
// of the interrupt handler.
class SpinLockNoLockInIntr : public Lock {
 public:
  void lock() override {
    if (!CPURegsAccessProvider::IsInterruptEnabled()) {
      return;
    }
    while (__atomic_test_and_set(&acquired, __ATOMIC_ACQUIRE)) {
    }
  }

  void unlock() override { __atomic_clear(&acquired, __ATOMIC_RELEASE); }

  bool try_lock() override {
    bool expected = false;
    return __atomic_compare_exchange_n(&acquired, &expected, true, false,
                                       __ATOMIC_ACQUIRE, __ATOMIC_ACQUIRE);
  }

 private:
  bool acquired = false;
};

class Mutex : public Lock {
 public:
  Mutex() : sema_(1) {}

  void lock() override { sema_.Down(); }
  void unlock() override { sema_.Up(); }

 private:
  Semaphore sema_;
};

}  // namespace Kernel

#endif
