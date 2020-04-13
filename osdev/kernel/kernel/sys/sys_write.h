#ifndef SYS_SYS_WRITE_H
#define SYS_SYS_WRITE_H

#include "../../std/printf.h"
#include "../../std/types.h"

namespace Kernel {

class SysWriteHandler {
 public:
  static SysWriteHandler& GetSysWriteHandler() {
    static SysWriteHandler handler;
    return handler;
  }

  size_t SysWrite(int fd, uint8_t* buf, size_t count) {
    // TODO Just a placeholder.
    if (fd == 1) {
      for (size_t i = 0; i < count; i++) {
        kprintf("%c", buf[i]);
      }
    }

    return count;
  }

  SysWriteHandler(const SysWriteHandler&) = delete;

 private:
  SysWriteHandler() {}
};
}  // namespace Kernel

#endif
