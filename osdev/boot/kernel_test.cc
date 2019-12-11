#include "kernel_test.h"
#include "string_util.h"

extern "C" void __cxa_pure_virtual() {
  while (1)
    ;
}

namespace Kernel {
namespace kernel_test {

void KernelTestRunner::RunTest() {
  for (int i = 0; i < num_tests_; i++) {
    kprintf("[ RUN      ]  %s.%s\n", tests_[i]->GetTestSuiteName(),
            tests_[i]->GetTestName());

    // Set current test.
    current_running = tests_[i];

    // Run test.
    tests_[i]->TestBody();

    if (!tests_[i]->GetBadCount()) {
      kprintf("[       OK ] ");
    } else {
      kprintf("[  FAILED  ] ");
    }
    kprintf("%s.%s\n", tests_[i]->GetTestSuiteName(), tests_[i]->GetTestName());
  }
}

template <>
void KernelTest::ExpectEq(const char* file, int line, const char* const& t,
                          const char* const& u) {
  if (strcmp(t, u) != 0) {
    kprintf("EXPECT_EQ Failed at: %s:%d\n", file, line);
    bad_count_++;
  }
}

KernelTest* current_running = nullptr;

}  // namespace kernel_test
}  // namespace Kernel
