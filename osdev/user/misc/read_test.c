#include <stdio.h>
#include <string.h>

#include "../test/ctest.h"

void test_fgetc() {
  FILE* file = fopen("/misc/short_text.txt", "r");
  ASSERT_TRUE(fgetc(file) == 's');
  ASSERT_TRUE(fgetc(file) == 'o');
  ASSERT_TRUE(fgetc(file) == 'm');
  ASSERT_TRUE(fgetc(file) == 'e');
  ASSERT_TRUE(fgetc(file) == 10); // Line Feed
  ASSERT_TRUE(fgetc(file) == EOF);
}

void test_fgets() {
  FILE* file = fopen("/misc/shakespeares.txt", "r");
  char line[10];

  fgets(line, sizeof(line), file);
  ASSERT_TRUE(strcmp(line, "Romeo and") == 0);

  fgets(line, sizeof(line), file);
  ASSERT_TRUE(strcmp(line, " Juliet\n") == 0);
}

void test_ftell() {
  FILE* file = fopen("/misc/shakespeares.txt", "r");
  ASSERT_TRUE(ftell(file) == 0);

  char line[100];
  fgets(line, sizeof(line), file);

  // We read the size of the buffer already.
  ASSERT_TRUE(ftell(file) == BUF_SIZE);
}

void test_filesize() {
  FILE* stream = fopen("/misc/shakespeares.txt", "r");
  fseek(stream, 0, SEEK_END);
  ASSERT_TRUE(ftell(stream) == 141695);
}

int main() {
  REGISTER_TEST(test_fgetc);
  REGISTER_TEST(test_fgets);
  REGISTER_TEST(test_ftell);
  REGISTER_TEST(test_filesize);

  RunTest();
  return 0;
}

