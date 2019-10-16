#include "kernel_test.h"
#include "list.h"

namespace Kernel {
namespace kernel_test {

TEST(KernelListTest, Simple) {
  KernelList<int> list;

  KernelListElement<int> elem1(&list);
  elem1.PushFront();

  KernelListElement<int> elem2(&list);
  elem2.PushFront();

  KernelListElement<int> elem3(&list);
  elem3.PushFront();
}

}  // namespace kernel_test
}  // namespace Kernel
