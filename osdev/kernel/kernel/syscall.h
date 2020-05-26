#include "../std/types.h"

namespace Kernel {

enum SyscallNumbers {
  SYS_EXIT = 0,
  SYS_READ,
  SYS_WRITE,
  SYS_FORK,  // Do not use
  SYS_EXEC,  // Do not use
  SYS_SPAWN = 5,
  SYS_WAITPID,
  SYS_OPEN
};

class SyscallManager {
 public:
  static SyscallManager& GetSyscallManager() {
    static SyscallManager syscall_manager;
    return syscall_manager;
  }

  int SyscallHandler(uint64_t syscall_num, uint64_t arg1, uint64_t arg2,
                     uint64_t arg3, uint64_t arg4, uint64_t arg5,
                     uint64_t arg6);

  void InitSyscall();

 private:
  SyscallManager();
};

}  // namespace Kernel

// This function will be used by syscall_handler defined as asm.
extern "C" int SyscallHandlerCaller(uint64_t syscall_num, uint64_t arg1,
                                    uint64_t arg2, uint64_t arg3, uint64_t arg4,
                                    uint64_t arg5, uint64_t arg6);
