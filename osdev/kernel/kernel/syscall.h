#include "../std/types.h"

namespace Kernel {

enum SyscallNumbers {
  SYS_EXIT = 0,
  SYS_READ,
  SYS_WRITE,
  SYS_FORK,
  SYS_EXEC,
  SYS_SPAWN = 5
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

  void SysExit(uint64_t exit_num);
  int SysWrite(uint32_t fd, const char* buf, size_t count);
  int SysFork();
};

}  // namespace Kernel

// This function will be used by syscall_handler defined as asm.
extern "C" int SyscallHandlerCaller(uint64_t syscall_num, uint64_t arg1,
                                    uint64_t arg2, uint64_t arg3, uint64_t arg4,
                                    uint64_t arg5, uint64_t arg6);
