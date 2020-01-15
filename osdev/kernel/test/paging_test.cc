#include "../kernel/paging.h"
#include "kernel_test.h"

namespace Kernel {
namespace kernel_test {
namespace {

TEST(PagingTest, SimpleFrame) {
  BuddyBlockAllocator alloc(/* start addr = */ 0, /* Up to 8 chunks */ 3,
                            /* size of chunk is */ 8);
  void* chunk1 = alloc.GetFrame(0);
  void* chunk2 = alloc.GetFrame(1);
  void* chunk3 = alloc.GetFrame(2);

  alloc.FreeFrame(chunk2);
  alloc.FreeFrame(chunk1);
  alloc.FreeFrame(chunk3);
}

}  // namespace
}  // namespace kernel_test
}  // namespace Kernel
