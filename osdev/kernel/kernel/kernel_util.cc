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
  while (1)
    ;
}

}  // namespace __
}  // namespace Kernel
