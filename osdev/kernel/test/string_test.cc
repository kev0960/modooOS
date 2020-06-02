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
  std::string_view view("this");
  KernelString str(view);

  EXPECT_EQ(str.size(), view.size());
}

TEST(KernelStringTest, TestSplit) {
  KernelString str("this    is  some  text ");
  auto sp = Split(str, ' ');

  EXPECT_EQ(sp.size(), 4ULL);
  EXPECT_EQ(sp[0], KernelString("this"));
  EXPECT_EQ(sp[1], KernelString("is"));
  EXPECT_EQ(sp[2], KernelString("some"));
  EXPECT_EQ(sp[3], KernelString("text"));
}

}  // namespace kernel_test
}  // namespace Kernel
