#include "kthread.h"
#include "kernel_test.h"

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
void test_2_func() {
  for (int i = 0; i < 100000000; i++) {
    test_2++;
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

  EXPECT_EQ(test_2, 500000000ull);
}

}  // namespace kernel_test
}  // namespace Kernel
