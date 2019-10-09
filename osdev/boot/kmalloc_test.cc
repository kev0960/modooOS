#include "kmalloc.h"
#include "kernel_test.h"

namespace Kernel {
namespace kernel_test {

TEST(kmallocTest, SimpleAllocAndFree) {
  void* mem1 = kmalloc(8);

  kernel_memory_manager.ShowDebugInfo();
  kfree(mem1);
  kernel_memory_manager.ShowDebugInfo();
}

TEST(kmallocTest, MergeTwoChunksToOne) {
  void* mem1 = kmalloc(8);
  void* mem2 = kmalloc(8);

  kfree(mem1);
  kernel_memory_manager.ShowDebugInfo();

  kfree(mem2);
  kernel_memory_manager.ShowDebugInfo();
}


}  // namespace kernel_test
}  // namespace Kernel
