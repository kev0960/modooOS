#include "kernel_util.h"

#include "cpu.h"
#include "cpu_context.h"
#include "printf.h"
#include "qemu_log.h"

namespace Kernel {
namespace __ {

void AssertTrue(bool condition, const char* func_name, int line) {
  if (!condition) {
    Panic(func_name, line);
  }
}

void Panic(const char* func_name, int line) {
  DisableInterrupt();
  QemuSerialLog::Logf("[CPU %d] Kernel Panic at : %s:%d \n",
                      CPUContextManager::GetCurrentCPUId(), func_name, line);
  PrintStackTrace();
  while (1)
    ;
}

}  // namespace __

void PrintStackTrace() {
  // Walk up the stack.
  uint64_t rbp;
  asm volatile(
      "mov %%rbp, %%rax\n"
      "mov %%rax, %0"
      : "=m"(rbp)::"%rax");

  do {
    uint64_t return_addr = *((uint64_t*)(rbp) + 1);
    QemuSerialLog::Logf("%lx [FrameAddr: %lx] \n", return_addr, rbp);

    uint64_t next_rbp = *(uint64_t*)(rbp);
    if (next_rbp == rbp) {
      break;
    }
    rbp = next_rbp;
  } while (rbp);
}

}  // namespace Kernel
