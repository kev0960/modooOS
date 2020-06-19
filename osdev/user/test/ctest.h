#ifndef TEST_CTEST_H
#define TEST_CTEST_H

#include <printf.h>

static int __num_tests = 0;

// Test functions should be in form of: void test_function()
typedef void (*TestFuncType)();

#define MAX_NUM_TEST_FUNC 100
static TestFuncType __test_functions[MAX_NUM_TEST_FUNC];
static char* __test_names[MAX_NUM_TEST_FUNC];

#define REGISTER_TEST(TEST_FUNC)          \
  __test_names[__num_tests] = #TEST_FUNC; \
  __test_functions[__num_tests++] = TEST_FUNC;

static int __num_violation = 0;

#define ASSERT_TRUE(expr)                                           \
  if (!(expr)) {                                                      \
    __num_violation++;                                              \
    printf("ASSERT_TRUE Failed at : %s:%d \n", __FILE__, __LINE__); \
  }

#define ASSERT_FALSE(expr)                                           \
  if (expr) {                                                        \
    __num_violation++;                                               \
    printf("ASSERT_FALSE Failed at : %s:%d \n", __FILE__, __LINE__); \
  }

void RunTest() {
  for (int i = 0; i < __num_tests; i++) {
    int prev_num_viol = __num_violation;
    printf("[Running] %s ----------- \n", __test_names[i]);
    __test_functions[i]();
    if (prev_num_viol == __num_violation) {
      printf("[Passed] %s \n", __test_names[i]);
    } else {
      printf("[Failed] %s \n", __test_names[i]);
    }
  }
  if (__num_violation == 0) {
    printf("All tests are passed :) \n");
  } else {
    printf("Some tests are failed :( \n");
  }
}

#endif
