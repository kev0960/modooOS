#ifndef SYS_SYS_CONSOLE_H
#define SYS_SYS_CONSOLE_H

#include "../console.h"
#include "../process.h"
#include "sys.h"
namespace Kernel {

// This is ModooOS's custom syscall. This will be used to send some commands to
// kernel console.

enum SysConsoleCommands { SET_NO_BUFFER, SET_LINE_BUFFER };

class SysConsoleHandler : public SyscallHandler<SysConsoleHandler> {
 public:
  size_t SysConsole(SysConsoleCommands command) {
    ASSERT(!KernelThread::CurrentThread()->IsKernelThread());
    Process* process = static_cast<Process*>(KernelThread::CurrentThread());

    // Make sure that the syscall is coming from current running foreground
    // process.
    auto& console = KernelConsole::GetKernelConsole();
    if (process != console.GetForegroundProcess()) {
      return 1;
    }

    if (command == SET_NO_BUFFER) {
      console.SetBufferingMode(KernelConsole::NO_BUFFER);
    } else if (command == SET_LINE_BUFFER) {
      console.SetBufferingMode(KernelConsole::LINE_BUFFER);
    }

    return 0;
  }
};

}  // namespace Kernel

#endif
