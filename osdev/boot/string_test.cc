#include "string.h"
#include "kernel_test.h"

namespace Kernel {
namespace kernel_test {

TEST(KernelStringTest, EmptyString) {
  KernelString str("");

  EXPECT_EQ(str.size(), 0u);
}

TEST(KernelStringTest, BasicStringCreateFromConstChar) {
  KernelString str("abc");

  EXPECT_EQ(str.size(), 3u);
}

TEST(KernelStringTest, BasicStringCreateFromStringView) {
  string_view view("this");
  KernelString str(view);

  EXPECT_EQ(str.size(), view.size());
}


}  // namespace kernel_test
}  // namespace Kernel
