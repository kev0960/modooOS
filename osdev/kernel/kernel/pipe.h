#ifndef PIPE_H
#define PIPE_H

#include "../std/map.h"
#include "file_descriptor.h"
#include "kthread.h"
#include "sync.h"

namespace Kernel {

class Pipe {
 public:
  Pipe() {}

  // Writes to the pipe.
  int Write(char* data, int len);

  // Read from the pipe.
  int Read(char* data);

 private:
  static constexpr int kPipeMaxSize = 4096;

  // For convenience, our pipe can hold up to 4096 bytes of data.
  char buf_[kPipeMaxSize];

  int buf_size_;

  MultiCoreSpinLock buf_access_lock_;
};

class PipeDescriptor : FileDescriptor {
 public:
  DescriptorType GetDescriptorType() final { return PIPE; }

  PipeDescriptor(Pipe* pipe) : pipe_(pipe) {}

  Pipe* GetPipe() { return pipe_; }

 private:
  Pipe* pipe_;
};

class PipeManager {
 public:
  static int CreatePipe(int pipe_fd[2]);

  static PipeManager& GetPipeManager() {
    static PipeManager pipe_manager;
    return pipe_manager;
  }

  static Pipe* GetPipe(int pipe_fd);

 private:
  static constexpr int kPipeStartId = 0x10000000;
  MultiCoreSpinLock pipe_create_lock_;
  int current_assigned_;

  std::map<int, Pipe*> id_to_pipe_;

  PipeManager() { current_assigned_ = kPipeStartId; }
};

}  // namespace Kernel

#endif
