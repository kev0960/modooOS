#include "pipe.h"

#include "qemu_log.h"

namespace Kernel {

int PipeManager::CreatePipe(int pipe_fd[2]) {
  PipeManager& pipe_manager = GetPipeManager();

  std::lock_guard<MultiCoreSpinLock> lk(pipe_manager.pipe_create_lock_);
  pipe_fd[0] = pipe_manager.current_assigned_++;
  pipe_fd[1] = pipe_manager.current_assigned_++;

  Pipe* pipe = new Pipe;

  pipe_manager.id_to_pipe_[pipe_fd[0]] = pipe;
  pipe_manager.id_to_pipe_[pipe_fd[1]] = pipe;

  return 0;
}

Pipe* PipeManager::GetPipe(int pipe_fd) {
  PipeManager& pipe_manager = GetPipeManager();

  std::lock_guard<MultiCoreSpinLock> lk(pipe_manager.pipe_create_lock_);
  auto itr = pipe_manager.id_to_pipe_.find(pipe_fd);
  if (itr != pipe_manager.id_to_pipe_.end()) {
    return (*itr).second;
  } else {
    return nullptr;
  }
}

int Pipe::Write(char* data, int len) {
  std::lock_guard<MultiCoreSpinLock> lk(buf_access_lock_);
  if (len >= kPipeMaxSize - buf_size_) {
    QemuSerialLog::Logf("Pipe is full!");
    return 0;
  }

  for (int i = 0; i < len; i++) {
    buf_[buf_size_ + i] = data[i];
  }

  buf_size_ += len;

  return len;
}

int Pipe::Read(char* data) {
  std::lock_guard<MultiCoreSpinLock> lk(buf_access_lock_);
  for (int i = 0; i < buf_size_; i++) {
    data[i] = buf_[i];
  }

  int prev_size = buf_size_;
  buf_size_ = 0;
  return prev_size;
}

}  // namespace Kernel
