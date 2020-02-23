#include "../std/types.h"

namespace Kernel {

class SyscallManager {
 public:
  static SyscallManager& GetSyscallManager() {
    static SyscallManager syscall_manager;
    return syscall_manager;
  }

  void SyscallHandler(uint64_t syscall_num);

 private:
  SyscallManager();
};

}  // namespace Kernel

// This function will be used by syscall_handler defined as asm.
extern "C" void SyscallHandlerCaller(uint64_t syscall_num, uint64_t arg1,
                                     uint64_t arg2, uint64_t arg3,
                                     uint64_t arg4, uint64_t arg5,
                                     uint64_t arg6);
