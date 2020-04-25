#include "sync.h"

#include "scheduler.h"
#include "../std/printf.h"

namespace Kernel {

void SpinLock::lock() {
  while (__atomic_test_and_set(&acquired, __ATOMIC_ACQUIRE)) {
  }
}

void SpinLock::unlock() { __atomic_clear(&acquired, __ATOMIC_RELEASE); }

bool SpinLock::try_lock() {
  bool expected = false;
  return __atomic_compare_exchange_n(&acquired, &expected, true, false,
                                     __ATOMIC_ACQUIRE, __ATOMIC_ACQUIRE);
}

void SpinLockNoLockInIntr::lock() {
  if (!CPURegsAccessProvider::IsInterruptEnabled()) {
    return;
  }
  while (__atomic_test_and_set(&acquired, __ATOMIC_ACQUIRE)) {
  }
}

void SpinLockNoLockInIntr::unlock() {
  __atomic_clear(&acquired, __ATOMIC_RELEASE);
}

bool SpinLockNoLockInIntr::try_lock() {
  bool expected = false;
  return __atomic_compare_exchange_n(&acquired, &expected, true, false,
                                     __ATOMIC_ACQUIRE, __ATOMIC_ACQUIRE);
}

void MultiCoreSpinLock::lock() {
  // Should not be called inside of an interrupt handler.
  ASSERT(CPURegsAccessProvider::IsInterruptEnabled());

  int cnt = 0;
  while (__atomic_test_and_set(&acquired, __ATOMIC_ACQUIRE)) {
    cnt++;

    // Spin for a while. If the lock is still not acquired, then just yield.
    if (cnt >= kMaxSpinCnt) {
      KernelThreadScheduler::GetKernelThreadScheduler().Yield();
      cnt = 0;
    }
  }
}

void MultiCoreSpinLock::unlock() {
  __atomic_clear(&acquired, __ATOMIC_RELEASE);
}

bool MultiCoreSpinLock::try_lock() {
  bool expected = false;
  return __atomic_compare_exchange_n(&acquired, &expected, true, false,
                                     __ATOMIC_ACQUIRE, __ATOMIC_ACQUIRE);
}
}  // namespace Kernel
