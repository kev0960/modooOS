#include "kernel_test.h"

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
    tests_[i]->TestBody();
    if (!tests_[i]->GetBadCount()) {
      kprintf("[       OK ] ");
    } else {
      kprintf("[  FAILED  ] ");
    }
    kprintf("%s.%s\n", tests_[i]->GetTestSuiteName(), tests_[i]->GetTestName());
  }
}
}  // namespace kernel_test
}  // namespace Kernel
