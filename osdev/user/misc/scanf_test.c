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

int main() {
  REGISTER_TEST(test_scanf_read_num);

  RunTest();

  return 0;
}
