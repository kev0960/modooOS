#include "sync.h"

#include "../std/printf.h"
#include "qemu_log.h"
#include "scheduler.h"

namespace Kernel {

void SpinLock::lock() {
  int cnt = 0;
  while (__atomic_test_and_set(&acquired, __ATOMIC_ACQUIRE)) {
    cnt++;
    if (cnt >= 10000) {
      QemuSerialLog::Logf("[SpinLock]Contention! %lx \n",
                          __builtin_return_address(0));
      cnt = 0;
    }
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
  // ASSERT(CPURegsAccessProvider::IsInterruptEnabled());

  int cnt = 0;
  int total_cnt = 0;
  while (true) {
    while (__atomic_load_n(&acquired, __ATOMIC_ACQUIRE)) {
      cnt++;
      total_cnt++;

      // Spin for a while. If the lock is still not acquired, then just yield.
      if (cnt >= kMaxSpinCnt) {
        // If not in the context of the interrupt handler, then yield.
        if (CPURegsAccessProvider::IsInterruptEnabled()) {
          KernelThreadScheduler::GetKernelThreadScheduler().Yield();
        }
        cnt = 0;
      }

      if (total_cnt > 100000000) {
        QemuSerialLog::Logf("[MulticoreSpinLock]Contention! %lx \n",
                            __builtin_return_address(0));
        total_cnt = 0;
      }
    }
    if (!__atomic_test_and_set(&acquired, __ATOMIC_ACQUIRE)) {
      break;
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
