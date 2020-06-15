#ifndef SYS_SYS_SBRK2_H
#define SYS_SYS_SBRK2_H

#include "../process.h"
#include "sys.h"

namespace Kernel {
namespace {

uint64_t RoundUpToMultipleOfFourKb(uint64_t bytes) {
  static constexpr size_t kFourKB = (1 << 12);
  if (bytes % kFourKB == 0) {
    return bytes;
  }
  return ((bytes / kFourKB) + 1) * kFourKB;
}

}  // namespace

class SysSbrkHandler : public SyscallHandler<SysSbrkHandler> {
 public:
  void* SysSbrk(intptr_t bytes) {
    ASSERT(!KernelThread::CurrentThread()->IsKernelThread());
    Process* process = static_cast<Process*>(KernelThread::CurrentThread());

    QemuSerialLog::Logf("Bytes : %d\n", bytes);
    void* prev_brk = process->GetHeapEnd();
    if (bytes < 0) {
      // TODO actually decrease the heap size.
      return prev_brk;
    }

    QemuSerialLog::Logf("Request size : %d", RoundUpToMultipleOfFourKb(bytes));
    process->IncreaseHeapSize(RoundUpToMultipleOfFourKb(bytes));
    return prev_brk;
  }
};

}  // namespace Kernel

#endif
