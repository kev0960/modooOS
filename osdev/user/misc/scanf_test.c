#include <stdio.h>

#include "../test/ctest.h"

void test_scanf_read_num() {
  int num = 0;

  const char* s1 = "123";
  sscanf(s1, "%d", &num);
  ASSERT_TRUE(num == 123);
  printf("num : %d", num);

  const char* s2 = "-123";
  sscanf(s2, "%d", &num);
  ASSERT_TRUE(num == -123);
  printf("num : %d", num);
}

void test_scanf_read_multiple_num() {
  int num[4];

  const char* s1 = "123 566 -123 +123";
  sscanf(s1, "%d%d%d%d", &num[0], &num[1], &num[2], &num[3]);
  ASSERT_TRUE(num[0] == 123);
  ASSERT_TRUE(num[1] == 566);
  ASSERT_TRUE(num[2] == -123);
  ASSERT_TRUE(num[3] == 123);
}

void test_scanf_read_multiple_hex() {
  int num[4];

  const char* s1 = "0xabc fff -123 -0x123";
  sscanf(s1, "%x%x%x%x", &num[0], &num[1], &num[2], &num[3]);
  ASSERT_TRUE(num[0] == 0xabc);
  ASSERT_TRUE(num[1] == 0xfff);
  ASSERT_TRUE(num[2] == -0x123);
  ASSERT_TRUE(num[3] == -0x123);
}

int main() {
  REGISTER_TEST(test_scanf_read_num);
  REGISTER_TEST(test_scanf_read_multiple_num);
  REGISTER_TEST(test_scanf_read_multiple_hex);

  RunTest();

  return 0;
}
