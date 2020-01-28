#include "../kernel/paging.h"
#include "kernel_test.h"

namespace Kernel {
namespace kernel_test {
namespace {

TEST(PagingTest, InitPaging) {
  KernelPageTable kernel_page_table;
  kernel_page_table.Init4KBPaging(0xFFFFFFFF80000000LL, (1 << 20));
}

}  // namespace
}  // namespace kernel_test
}  // namespace Kernel
