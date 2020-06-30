#ifndef SYS_SYS_CONSOLE_H
#define SYS_SYS_CONSOLE_H

#include "../console.h"
#include "../process.h"
#include "sys.h"

namespace Kernel {

// This is ModooOS's custom syscall. This will be used to send some commands to
// kernel console.

enum SysConsoleCommands {
  SET_NO_BUFFER = 1,
  SET_LINE_BUFFER = 2,
  SET_ASCII_ONLY = 4,
  SET_EVERYTHING = 8,
  SET_BLOCKING_IO = 16,
  SET_NON_BLOCKING_IO = 32,
  SET_RECORD_UP = 64,
};

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

    if (command & SET_NO_BUFFER) {
      console.SetBufferingMode(KernelConsole::NO_BUFFER);
    }
    if (command & SET_LINE_BUFFER) {
      console.SetBufferingMode(KernelConsole::LINE_BUFFER);
    }
    if (command & SET_ASCII_ONLY) {
      console.SetKeystrokeMode(KernelConsole::ASCII_ONLY);
    }
    if (command & SET_EVERYTHING) {
      console.SetKeystrokeMode(KernelConsole::EVERYTHING);
    }
    if (command & SET_RECORD_UP) {
      console.SetKeystrokeMode(KernelConsole::RECORD_UP);
    }
    if (command & SET_BLOCKING_IO) {
      console.SetBlockingMode(true);
    }
    if (command & SET_NON_BLOCKING_IO) {
      console.SetBlockingMode(false);
    }

    return 0;
  }
};

}  // namespace Kernel

#endif
