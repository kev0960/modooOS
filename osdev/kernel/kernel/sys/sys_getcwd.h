#ifndef SYS_GETCWD_H
#define SYS_GETCWD_H

#include "../process.h"
#include "sys.h"

namespace Kernel {

class SysGetCWDHandler : public SyscallHandler<SysGetCWDHandler> {
 public:
  char* SysGetCWD(char* buf, size_t size) {
    ASSERT(!KernelThread::CurrentThread()->IsKernelThread());
    Process* process = static_cast<Process*>(KernelThread::CurrentThread());

    const auto& current_working_dir = process->GetWorkingDir();
    if (current_working_dir.size() + 1 >= size) {
      return nullptr;
    }

    for (size_t i = 0; i < current_working_dir.size(); i++) {
      buf[i] = current_working_dir.at(i);
    }

    // End with Null terminator.
    buf[current_working_dir.size()] = '\0';

    return buf;
  }
};

}  // namespace Kernel

#endif
