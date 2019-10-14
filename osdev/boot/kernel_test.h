#ifndef KERNEL_TEST_H
#define KERNEL_TEST_H

#include "printf.h"

namespace Kernel {
namespace kernel_test {

constexpr int kMaxNumTest = 100;

// Kernel test framework. This is very similar to the unit test framework.
// The only difference is that our test framework runs natively inside of the
// kernel where as the unit testing framework runs on host OS.
//
// That means, we have to actually execute OS to run the Kernel test.
class KernelTest;

class KernelTestRunner {
 public:
  static KernelTestRunner& GetTestRunner() {
    static KernelTestRunner test_runner;
    return test_runner;
  }

  void AddTest(KernelTest* test) {
    if (num_tests_ >= kMaxNumTest) {
      printf("Max Test Number Exceeded!");
      return;
    }

    tests_[num_tests_++] = test;
  }

  void RunTest();
  KernelTestRunner(const KernelTestRunner&) = delete;
  void operator=(const KernelTestRunner&) = delete;

 public:
  KernelTestRunner() : num_tests_(0) {}

  KernelTest* tests_[kMaxNumTest];
  int num_tests_;
};

class KernelTest {
 public:
  KernelTest(const char* test_suite_name, const char* test_name)
      : test_suite_name_(test_suite_name), test_name_(test_name) {
    RegisterTest();
  }
  virtual void TestBody() = 0;

  void RegisterTest() {
    // Register test to the test runner.
    KernelTestRunner::GetTestRunner().AddTest(this);
  }

  template <typename T, typename U>
  void ExpectEq(const char* file, int line, const T& t, const U& u) {
    if (t != u) {
      printf("EXPECT_EQ Failed at: %s:%d\n", file, line);
      bad_count_++;
    }
  }

  void ExpectTrue(const char* file, int line, bool t) {
    if (!t) {
      printf("EXPECT_TRUE Failed at: %s:%d\n", file, line);
      bad_count_++;
    }
  }

  void ExpectFalse(const char* file, int line, bool t) {
    if (t) {
      printf("EXPECT_FALSE Failed at: %s:%d\n", file, line);
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

#define TEST(test_suite_name, test_name)                                   \
  class TEST_CLASS_NAME_(test_suite_name, test_name) : public KernelTest { \
   public:                                                                 \
    TEST_CLASS_NAME_(test_suite_name, test_name)                           \
    (const char* tsn, const char* ts) : KernelTest(tsn, ts) {}             \
    void TestBody() override;                                              \
  };                                                                       \
  TEST_CLASS_NAME_(test_suite_name, test_name)                             \
  TEST_CLASS_OBJECT_(test_suite_name, test_name)                           \
  (#test_suite_name, #test_name);                                          \
  void TEST_CLASS_NAME_(test_suite_name, test_name)::TestBody()

#define EXPECT_EQ(T, U) ExpectEq(__FILE__, __LINE__, T, U);
#define EXPECT_TRUE(T) ExpectTrue(__FILE__, __LINE__, T);
#define EXPECT_FALSE(T) ExpectFalse(__FILE__, __LINE__, T);

extern "C" void __cxa_pure_virtual();

}  // namespace kernel_test
}  // namespace Kernel

#endif
