
#include <printf.h>
#include <string.h>
#include <syscall.h>

#include "../test/ctest.h"

void test_memcmp() {
  char lhs[] = "0123456789";
  char rhs[] = "0123456789abc";

  ASSERT_TRUE(memcmp(lhs, rhs, sizeof lhs - 1) == 0);
  ASSERT_TRUE(memcmp(lhs, rhs, sizeof lhs) != 0);
  ASSERT_TRUE(memcmp(lhs, rhs, sizeof rhs) != 0);
}

void test_memcpy() {
  char src[] = "0123456";
  char dest[3];

  ASSERT_TRUE(memcpy(dest, src, sizeof dest) == dest);
  ASSERT_TRUE(memcmp(dest, "012", 3) == 0);

  char s[] = "0123456789";
  memcpy(s + 4, s + 3, 3);
  ASSERT_TRUE(memcmp(s, "0123333789", 10) == 0);
}

void test_memmove() {
  char s[] = "0123456789";
  ASSERT_TRUE(memmove(s + 4, s + 3, 3) == (s + 4));
  ASSERT_TRUE(memcmp(s, "0123345789", 10) == 0);

  char s2[] = "0123456789";
  ASSERT_TRUE(memmove(s2 + 3, s2 + 4, 3) == (s2 + 3));
  ASSERT_TRUE(memcmp(s2, "0124566789", 10) == 0);

  char lhs[] = "abcd";
  char rhs[] = "def";
  ASSERT_TRUE(memmove(lhs, rhs, 3) == lhs);
  ASSERT_TRUE(memcmp(lhs, "defd", 4) == 0);
}

void test_strcmp() {
  char lhs[] = "abcdefg";
  char rhs[] = "abcdfg";

  ASSERT_TRUE(strcmp(lhs, rhs) < 0);

  char rhs2[] = "abcdefg";
  ASSERT_TRUE(strcmp(lhs, rhs2) == 0);

  char rhs3[] = "abcddfg";
  ASSERT_TRUE(strcmp(lhs, rhs3) > 0);
}

void test_strncmp() {
  char lhs[] = "abcdefg";
  char rhs[] = "abcdfg";

  ASSERT_TRUE(strncmp(lhs, rhs, 4) == 0);
  ASSERT_TRUE(strncmp(lhs, rhs, 5) < 0);
  ASSERT_TRUE(strncmp(lhs, rhs, 10) < 0);
}

void test_strlen() {
  ASSERT_TRUE(strlen("") == 0);
  ASSERT_TRUE(strlen("abc") == 3);
}

void test_strchr() {
  char empty_string[] = "";
  ASSERT_TRUE(strchr(empty_string, 'a') == 0);

  char s[] = "abcd";
  ASSERT_TRUE((strchr(s, 'c') == (s + 2)));
  ASSERT_TRUE((strchr(s, '\0') == (s + 4)));
}

int main() {
  REGISTER_TEST(test_memcmp);
  REGISTER_TEST(test_memcpy);
  REGISTER_TEST(test_memmove);
  REGISTER_TEST(test_strcmp);
  REGISTER_TEST(test_strncmp);
  REGISTER_TEST(test_strlen);
  REGISTER_TEST(test_strchr);

  RunTest();

  return 0;
}
