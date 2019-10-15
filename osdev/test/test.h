// Header-Free Test Framework specifically for OSDEV.

#ifndef TEST_H
#define TEST_H

#include "color.h"
#include "printf.h"

#define MAX_NUM_TEST_AVAILABLE 100

extern "C" void __cxa_pure_virtual();
class Test;

using TestFunctionType = void (*)();

class TestRunner {
 public:
  static TestRunner& GetTestRunner() {
    static TestRunner test_runner;
    return test_runner;
  }

  void AddTest(Test* test) {
    if (num_tests_ >= MAX_NUM_TEST_AVAILABLE) {
      return;
    }

    tests_[num_tests_++] = test;
  }

  void RunTest();
  TestRunner(const TestRunner&) = delete;
  void operator=(const TestRunner&) = delete;

 public:
  TestRunner() : num_tests_(0) {}

  Test* tests_[MAX_NUM_TEST_AVAILABLE];
  int num_tests_;
};

class Test {
 public:
  Test(const char* test_suite_name, const char* test_name)
      : test_suite_name_(test_suite_name), test_name_(test_name) {
    RegisterTest();
  }
  virtual void TestBody() = 0;

  void RegisterTest() {
    // Register test to the test runner.
    TestRunner::GetTestRunner().AddTest(this);
  }

  template <typename T, typename U>
  void ExpectEq(const char* file, int line, const T& t, const U& u) {
    if (t != u) {
      kprintf("EXPECT_EQ Failed at: %s:%d\n", file, line);
      bad_count_++;
    }
  }

  const char* GetTestSuiteName() const { return test_suite_name_; }
  const char* GetTestName() const { return test_name_; }
  int GetBadCount() const { return bad_count_; }

 private:
  const char* test_suite_name_;
  const char* test_name_;

  int bad_count_ = 0;
};

#define TEST_CLASS_NAME_(test_suite_name, test_name) \
  test_suite_name##_##test_name##_Test

#define TEST_CLASS_OBJECT_(test_suite_name, test_name) \
  test_suite_name##_##test_name##_Test_obj

#define TEST(test_suite_name, test_name)                             \
  class TEST_CLASS_NAME_(test_suite_name, test_name) : public Test { \
   public:                                                           \
    TEST_CLASS_NAME_(test_suite_name, test_name)                     \
    (const char* tsn, const char* ts) : Test(tsn, ts) {}             \
    void TestBody() override;                                        \
  };                                                                 \
  TEST_CLASS_NAME_(test_suite_name, test_name)                       \
  TEST_CLASS_OBJECT_(test_suite_name, test_name)                     \
  (#test_suite_name, #test_name);                                    \
  void TEST_CLASS_NAME_(test_suite_name, test_name)::TestBody()

#define EXPECT_EQ(T, U) ExpectEq(__FILE__, __LINE__, T, U);

extern "C" void __cxa_pure_virtual() {
  while (1)
    ;
}

void TestRunner::RunTest() {
  for (int i = 0; i < num_tests_; i++) {
    kprintf(KGRN "[ RUN      ] " KWHT "%s.%s\n", tests_[i]->GetTestSuiteName(),
           tests_[i]->GetTestName());
    tests_[i]->TestBody();
    if (!tests_[i]->GetBadCount()) {
      kprintf(KGRN "[       OK ] " KWHT);
    } else {
      kprintf(KRED "[  FAILED  ] " KWHT);
    }
    kprintf("%s.%s\n", tests_[i]->GetTestSuiteName(), tests_[i]->GetTestName());
  }
}

extern "C" int TestMain() {
  kprintf("Starting Test ---- \n");
  TestRunner::GetTestRunner().RunTest();
  return 0;
}

#endif
