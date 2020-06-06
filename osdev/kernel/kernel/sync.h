#ifndef SYNC_H
#define SYNC_H

#include "../std/types.h"
#include "cpu.h"
#include "kthread.h"
#include "semaphore.h"

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
  Lock();
  Lock(const char* lock_name);

  virtual void lock() = 0;
  virtual void unlock() = 0;
  virtual bool try_lock() = 0;

  virtual ~Lock() = default;

  const char* lock_name_;
};

// IRQ based lock. It disables the entire interrupt so that it can run
// exclusively.
class IrqLock : public Lock {
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
  SpinLock() : Lock() {}
  SpinLock(const char* lock_name) : Lock(lock_name) {}

  void lock() override;
  void unlock() override;
  bool try_lock() override;

 private:
  bool acquired = false;
  KernelThread* holder_;
};

// This spinlock can be used even when the spinlock needs to be acuqired inside
// of the interrupt handler.
class SpinLockNoLockInIntr : public Lock {
 public:
  void lock() override;
  void unlock() override;
  bool try_lock() override;

 private:
  bool acquired = false;
};

class MultiCoreSpinLock : public Lock {
 public:
  static const int kMaxSpinCnt = 1000;

  MultiCoreSpinLock(const char* lock_name) : Lock(lock_name) {}
  MultiCoreSpinLock() : Lock() {}

  void lock() override;
  void unlock() override;
  bool try_lock() override;

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

// This acts like a semaphore but does not have waiting queue.
class MultiCoreSema {
 public:
  MultiCoreSema(int cnt) : cnt_(cnt) {}

  void Down();
  void Up();

 private:
  MultiCoreSpinLock lock_;

  int cnt_;
};

}  // namespace Kernel

#endif
