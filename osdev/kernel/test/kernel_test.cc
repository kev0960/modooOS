#include "kernel_test.h"

#include "../kernel/qemu_log.h"
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
    QemuSerialLog::Logf("[ RUN      ]  %s.%s\n", tests_[i]->GetTestSuiteName(),
                        tests_[i]->GetTestName());

    // Set current test.
    current_running = tests_[i];

    // Run test.
    tests_[i]->TestBody();

    if (!tests_[i]->GetBadCount()) {
      kprintf("[       OK ] ");
      QemuSerialLog::Logf("[       OK ] ");
    } else {
      kprintf("[  FAILED  ] ");
      QemuSerialLog::Logf("[  FAILED  ] ");
    }
    kprintf("%s.%s\n", tests_[i]->GetTestSuiteName(), tests_[i]->GetTestName());
    QemuSerialLog::Logf("%s.%s\n", tests_[i]->GetTestSuiteName(),
                        tests_[i]->GetTestName());
  }
}

KernelTest* current_running = nullptr;

}  // namespace kernel_test
}  // namespace Kernel
