#include "kthread.h"
#include "kernel_test.h"
#include "scheduler.h"
#include "sync.h"

namespace Kernel {
namespace kernel_test {

// WARNING -------------------
// Those tests MUST be run in order.

// Check CurrentThread() works as intended.
TEST(KernelThreadTest, GetCurrentThread) {
  // Id of the init thread should be 0.
  EXPECT_EQ(KernelThread::CurrentThread()->Id(), 0ul)
}

int simple_run_cnt = 0;
void func1() {
  for (int i = 0; i < 1000; i++) {
    simple_run_cnt++;
  }
}

TEST(KernelThreadTest, SimpleRun) {
  KernelThread thread(func1);
  EXPECT_EQ(simple_run_cnt, 0);

  thread.Start();

  // Wait until it finishes.
  thread.Join();

  EXPECT_EQ(simple_run_cnt, 1000);
}

uint64_t test_2 = 0;
SpinLock lock;
void __attribute__((optimize("O0"))) test_2_func() {
  for (int i = 0; i < 1000000; i++) {
    lock.lock();
    test_2++;
    lock.unlock();
  }
}

TEST(KernelThreadTest, MultipleThreads) {
  KernelThread thread1(test_2_func);
  KernelThread thread2(test_2_func);
  KernelThread thread3(test_2_func);
  KernelThread thread4(test_2_func);
  KernelThread thread5(test_2_func);

  EXPECT_EQ(test_2, 0ull);

  thread1.Start();
  thread2.Start();
  thread3.Start();
  thread4.Start();
  thread5.Start();

  // Wait until it finishes.
  thread1.Join();
  thread2.Join();
  thread3.Join();
  thread4.Join();
  thread5.Join();

  EXPECT_EQ(test_2, 5000000ull);
}

void __attribute__((optimize("O0"))) test_2_func2() {
  for (int i = 0; i < 10000; i++) {
    int total = i;
    for (int j = 0; j < 10000; j++) {
      total += j;
    }
    if (total != 5000 * 9999 + i) {
      kprintf("WTF?");
    }
  }
}

TEST(KernelThreadTest, MultipleThreadsIndep) {
  KernelThread thread1(test_2_func2);
  KernelThread thread2(test_2_func2);
  KernelThread thread3(test_2_func2);
  KernelThread thread4(test_2_func2);
  KernelThread thread5(test_2_func2);

  thread1.Start();
  thread2.Start();
  thread3.Start();
  thread4.Start();
  thread5.Start();

  // Wait until it finishes.
  thread1.Join();
  thread2.Join();
  thread3.Join();
  thread4.Join();
  thread5.Join();
}

void __attribute__((optimize("O0"))) yield_func(int i) {
  int total = 0;
  for (int j = 0; j < 10000; j++) {
    total += j;
    if (j % i == 0) {
      KernelThreadScheduler::GetKernelThreadScheduler().Yield();
    }
  }
  if (total != 5000 * 9999) {
    kprintf("??? %d \n", total);
  }
}

TEST(KernelThreadTest, YieldTest) {
  KernelThread thread1([]() { yield_func(2); });
  KernelThread thread2([]() { yield_func(3); });
  KernelThread thread3([]() { yield_func(4); });
  KernelThread thread4([]() { yield_func(5); });
  KernelThread thread5([]() { yield_func(6); });
  thread1.Start();
  thread2.Start();
  thread3.Start();
  thread4.Start();
  thread5.Start();

  thread1.Join();
  thread2.Join();
  thread3.Join();
  thread4.Join();
  thread5.Join();
}

Semaphore sema(1);

static int test_3 = 0;
void __attribute__((optimize("O0"))) test_3_func() {
  for (int i = 0; i < 2000000; i++) {
    sema.Down();
    test_3++;
    sema.Up();
  }
}

TEST(KernelThreadTest, Sema) {
  KernelThread thread1(test_3_func);
  KernelThread thread2(test_3_func);
  KernelThread thread3(test_3_func);
  KernelThread thread4(test_3_func);
  KernelThread thread5(test_3_func);
  KernelThread thread6(test_3_func);

  thread1.Start();
  thread2.Start();
  thread3.Start();
  thread4.Start();
  thread5.Start();
  thread6.Start();

  thread1.Join();
  thread2.Join();
  thread3.Join();
  thread4.Join();
  thread5.Join();

  EXPECT_EQ(test_3, 12000000)
}
}  // namespace kernel_test
}  // namespace Kernel
