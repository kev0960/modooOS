#ifndef QEMU_LOG
#define QEMU_LOG

#include "../std/printf.h"
#include "io.h"
#include "sync.h"

namespace Kernel {

class QemuSerialLog {
 public:
  static void Log(const char* s) {
    GetQemuSerialLog().sp_.lock();
    while (*s) {
      outb(0x3F8, *s);
      s++;
    }
    GetQemuSerialLog().sp_.unlock();
  }

  static void Logf(const char* format, ...) {
    char buf[1000];
    va_list args;
    va_start(args, format);

    int written = vsnprintf(buf, 1000, format, args);
    buf[written] = '\0';

    va_end(args);

    GetQemuSerialLog().sp_.lock();
    for (int i = 0; i < written; i++) {
      outb(0x3F8, buf[i]);
    }
    GetQemuSerialLog().sp_.unlock();
  }

 private:
  static QemuSerialLog& GetQemuSerialLog() {
    static QemuSerialLog lg;
    return lg;
  }

  QemuSerialLog() : sp_("QemuLogLock") {}

  SpinLock sp_;
};

}  // namespace Kernel

#endif
