#include "pipe.h"

#include "../std/algorithm.h"
#include "qemu_log.h"
#include "scheduler.h"

namespace Kernel {

int Pipe::Write(char* data, int len) {
  QemuSerialLog::Logf("Write PIPE!!! : %s \n", data);
  if (len > kPipeMaxSize) {
    QemuSerialLog::Logf("Data size is too large!");
    return 0;
  }

  // Wait until there is nothing left in the pipe.
  while (true) {
    buf_access_lock_.lock();
    if (size_ > 0) {
      buf_access_lock_.unlock();
      KernelThreadScheduler::GetKernelThreadScheduler().Yield();
      continue;
    }

    for (int i = 0; i < len; i++) {
      buf_[i] = data[i];
    }

    size_ = len;
    buf_access_lock_.unlock();
    return len;
  }
}

int Pipe::Read(char* data, size_t count) {
  // Wait until there is some data in the pipe.
  while (true) {
    buf_access_lock_.lock();
    if (size_ == 0) {
      buf_access_lock_.unlock();
      KernelThreadScheduler::GetKernelThreadScheduler().Yield();
      continue;
    }

    int actually_read = min((int)count, size_);
    for (int i = 0; i < actually_read; i++) {
      data[i] = buf_[i];
    }

    size_ = 0;
    buf_access_lock_.unlock();

    return actually_read;
  }
}

int PipeDescriptorReadEnd::Read(char* data, size_t count) {
  return pipe_->Read(data, count);
}

int PipeDescriptorWriteEnd::Write(char* data, int len) {
  return pipe_->Write(data, len);
}

}  // namespace Kernel
