#include "../boot/string_util.h"
#include "test.h"

TEST(StringUtilTest, StrLen) {
  EXPECT_EQ(Kernel::strlen(""), 0);
  EXPECT_EQ(Kernel::strlen("some string"), 11);
}
