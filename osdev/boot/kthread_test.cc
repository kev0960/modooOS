#include "kthread.h"
#include "kernel_test.h"

namespace Kernel {
namespace kernel_test {

// WARNING -------------------
// Those tests MUST be ran in order.

// Check CurrentThread() works as intended.
TEST(KernelThreadTest, GetCurrentThread) {
  KernelThread::InitThread();

  // Id of the init thread should be 0.
  EXPECT_EQ(KernelThread::CurrentThread()->Id(), 0ul)
}

}  // namespace kernel_test
}  // namespace Kernel
