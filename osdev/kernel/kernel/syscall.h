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
  SYS_OPEN,
  SYS_PIPE = 8,
  SYS_DUP2,
  SYS_STAT = 10,
  SYS_SBRK,
  SYS_GETDENTS,
  SYS_GETCWD,
  SYS_CONSOLE,
  SYS_SCREEN = 15,
  SYS_USLEEP,
  SYS_MSTICK,
  SYS_PREAD,
  SYS_LSEEK
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
