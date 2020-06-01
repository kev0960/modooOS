#include "console.h"

#include "process.h"
#include "scheduler.h"

namespace Kernel {

void KernelConsole::InitKernelConsole() {
  Process* console_process = new Process(
      nullptr, "kconsole", [] { KernelConsole::GetKernelConsole().Run(); });
  console_process->Start();
}

KernelConsole::KernelConsole() {
  line_buffer_ = reinterpret_cast<char*>(kmalloc(kLineBufferSize));

  input_buffer_ = reinterpret_cast<char*>(kmalloc(kInputLineBufferSize));
  input_buffer_size_ = 0;

  received_keyinfo_queue.reserve(kKeyStrokeBufferSize);

  // Pre-fill the queue with the dummy values.
  for (int i = 0; i < kKeyStrokeBufferSize; i++) {
    received_keyinfo_queue.push_back(KeyStroke());
  }

  current_read_index_ = 0;
  current_write_index_ = 0;

  is_running_ = false;
}

void KernelConsole::AddKeyStroke(const KeyStroke& key) {
  if (current_write_index_ == kKeyStrokeBufferSize) {
    current_write_index_ = 0;
  }
  received_keyinfo_queue[current_write_index_] = key;
}

int KernelConsole::ReadKeyStroke(std::vector<KeyStroke>* strokes) {
  if (current_write_index_ == current_read_index_) {
    return 0;
  }
  int num_to_read = current_write_index_ - current_read_index_;

  // write_index wrapped around the read index.
  if (num_to_read < 0) {
    num_to_read += kKeyStrokeBufferSize;
  }

  for (int i = 0; i < num_to_read; i++) {
    if (num_to_read + current_read_index_ < kKeyStrokeBufferSize) {
      (*strokes)[num_to_read] =
          received_keyinfo_queue[num_to_read + current_read_index_];
    } else {
      (*strokes)[num_to_read] =
          received_keyinfo_queue[num_to_read + current_read_index_ -
                                 kKeyStrokeBufferSize];
    }
  }

  // We cannot just set it as current_write_index_ becaause new keystroke might
  // have been added the queue in the meantime.
  current_read_index_ += num_to_read;
  if (current_read_index_ >= kKeyStrokeBufferSize) {
    current_read_index_ -= kKeyStrokeBufferSize;
  }

  return num_to_read;
}

void KernelConsole::Run() {
  is_running_ = true;

  while (true) {
    int num_received = ReadKeyStroke(&received_keyinfo_queue);

    // Yield if nothing is received.
    if (num_received == 0) {
      KernelThreadScheduler::GetKernelThreadScheduler().Yield();
      continue;
    }

    if (fg_process_ == nullptr) {
      // Then this is an input to the console.
    } else {
      // Otherwise pass the input to the foreground process.
    }
  }
}

}  // namespace Kernel
