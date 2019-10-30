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

TEST(KernelThreadTest, SimpleRun) {
}

}  // namespace kernel_test
}  // namespace Kernel
