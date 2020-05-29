#ifndef PIPE_H
#define PIPE_H

#include "../std/map.h"
#include "file_descriptor.h"
#include "kthread.h"
#include "sync.h"

namespace Kernel {

class Pipe {
 public:
  // Writes to the pipe. Will wait until there is nothing left in the pipe.
  int Write(char* data, int len);

  // Read from the pipe. Will wait until there is something in the pipe.
  // If count < size_, then any remaining data will be dropped from pipe.
  // Hence, to make sure the user reads everything, it needs to provide at least
  // kPipeMaxSize buffer.
  int Read(char* data, size_t count);

  Pipe() = default;
  ~Pipe() = default;

 private:
  static constexpr int kPipeMaxSize = 4096;

  // Our pipe can hold up to 4096 bytes of data.
  char buf_[kPipeMaxSize];

  // Current buffer size.
  int size_;

  MultiCoreSpinLock buf_access_lock_;
};

class PipeDescriptorReadEnd : public FileDescriptor {
 public:
  PipeDescriptorReadEnd(Pipe* pipe) : pipe_(pipe) {}

  DescriptorType GetDescriptorType() final { return PIPE_READ; }

  int Read(char* data, size_t count);

 private:
  Pipe* pipe_;
};

class PipeDescriptorWriteEnd : public FileDescriptor {
 public:
  PipeDescriptorWriteEnd(Pipe* pipe) : pipe_(pipe) {}

  DescriptorType GetDescriptorType() final { return PIPE_WRITE; }

  int Write(char* data, int len);

 private:
  Pipe* pipe_;
};

}  // namespace Kernel

#endif
