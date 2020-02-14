#include "kernel_util.h"

#include "cpu.h"
#include "printf.h"

namespace Kernel {
namespace __ {

void AssertTrue(bool condition, const char* func_name, int line) {
  if (!condition) {
    Panic(func_name, line);
  }
}

void Panic(const char* func_name, int line) {
  DisableInterrupt();
  kprintf("Kernel Panic at : %s:%d", func_name, line);
  while(1);
}

}  // namespace __
}  // namespace Kernel
