#include "fpu.h"

#include "qemu_log.h"

namespace Kernel {

void FPUManager::InitFPU() {
  asm volatile(
      "mov %%cr0, %%rax\n"
      "and $0xFFFB, %%ax\n"  // Clear coprocessor emulation CR0.EM
      "or $0x2, %%ax\n"      // Set Coprocessor monitoring CR0.MP
      "mov %%rax, %%cr0\n"
      "mov %%cr4, %%rax\n"
      "or $0x600, %%ax\n"  // Set both CR4.OSFXSR and CR4.OSXMMEXCPT
      "mov %%rax, %%cr4\n" ::
          :);
  HasSSE();
}

bool FPUManager::HasSSE() {
  int zero_flag;
  asm volatile(
      "mov $0x1, %%eax\n"
      "cpuid\n"
      "test $0x2000, %%edx\n"
      : "=@ccz"(zero_flag)::);
  QemuSerialLog::Logf("Has SSE support? %d", !zero_flag);
  return !zero_flag;
}

}  // namespace Kernel
