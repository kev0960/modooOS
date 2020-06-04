#include "console.h"

#include "../std/string_view.h"
#include "process.h"
#include "scheduler.h"
#include "string.h"
#include "vga_output.h"

namespace Kernel {

void KernelConsole::InitKernelConsole() {
  KernelThread* console_thread =
      new KernelThread([] { KernelConsole::GetKernelConsole().Run(); });
  console_thread->Start();

  // This thread will keep monitor the terminal output buffer and print anything
  // that shows up there.
  KernelThread* print_terminal_thread = new KernelThread(
      [] { KernelConsole::GetKernelConsole().PrintTermOutputBuffer(); });
  print_terminal_thread->Start();
}

KernelConsole::KernelConsole() {
  input_buffer_ = reinterpret_cast<char*>(kmalloc(kInputLineBufferSize));
  input_buffer_size_ = 0;

  term_output_buffer_ =
      reinterpret_cast<char*>(kmalloc(kTerminalOutputBufferSize));
  term_output_read_index_ = 0;
  term_output_write_index_ = 0;
  term_output_buffer_to_print_ =
      reinterpret_cast<char*>(kmalloc(kTerminalOutputBufferSize + 1));

  received_keyinfo_queue.reserve(kKeyStrokeBufferSize);

  // Pre-fill the queue with the dummy values.
  for (int i = 0; i < kKeyStrokeBufferSize; i++) {
    received_keyinfo_queue.push_back(KeyStroke());
  }

  current_read_index_ = 0;
  current_write_index_ = 0;

  is_running_ = false;
  should_show_shell_prefix_ = true;
}

void KernelConsole::AddKeyStroke(const KeyStroke& key) {
  // Handle some commands that requires immediate intervertion (e.g ctrl-c to
  // kill the current running process.)
  if (key.c == 'c' && key.is_ctrl_down) {
    if (fg_process_ != nullptr) {
      fg_process_->MakeTerminate();
    }
  }

  if (current_write_index_ == kKeyStrokeBufferSize) {
    current_write_index_ = 0;
  }

  received_keyinfo_queue[current_write_index_] = key;
  current_write_index_++;
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
    if (i + current_read_index_ < kKeyStrokeBufferSize) {
      (*strokes)[i] = received_keyinfo_queue[i + current_read_index_];
    } else {
      (*strokes)[i] = received_keyinfo_queue[i + current_read_index_ -
                                             kKeyStrokeBufferSize];
    }
  }

  // We cannot just set it as current_write_index_ because new keystroke might
  // have been added the queue in the meantime.
  current_read_index_ += num_to_read;
  if (current_read_index_ >= kKeyStrokeBufferSize) {
    current_read_index_ -= kKeyStrokeBufferSize;
  }

  return num_to_read;
}

void KernelConsole::Run() {
  is_running_ = true;

  auto& vga_output = VGAOutput::GetVGAOutput();

  QemuSerialLog::Logf("STart Kernel console!");

  while (true) {
    if (should_show_shell_prefix_) {
      kprintf("root:/# ");
      should_show_shell_prefix_ = false;
    }

    int num_received = ReadKeyStroke(&received_keyinfo_queue);
    // Yield if nothing is received.
    if (num_received == 0) {
      KernelThreadScheduler::GetKernelThreadScheduler().Yield();
      continue;
    }

    if (fg_process_ == nullptr) {
      // Then this is an input to the console.
      vga_output.PrintKeyStrokes(received_keyinfo_queue, 0, num_received);

      // Fill buffer and parse if needed.
      FillInputBufferAndParse(num_received);
    } else {
      // Otherwise pass the input to the foreground process.
    }
  }
}

void KernelConsole::FillInputBufferAndParse(int num_received) {
  for (int i = 0; i < num_received; i++) {
    if (received_keyinfo_queue[i].c == KEY_CODES::ENTER) {
      // Parse the input.
      DoParse();

      should_show_shell_prefix_ = true;
    } else if (received_keyinfo_queue[i].c == KEY_CODES::BACKSPACE) {
      if (input_buffer_size_ > 0) {
        input_buffer_size_--;
      }
    } else {
      char c = received_keyinfo_queue[i].ToChar();
      if (c != 0) {
        input_buffer_[input_buffer_size_] = c;
        input_buffer_size_++;
      }
    }

    if (input_buffer_size_ >= kInputLineBufferSize) {
      break;
    }
  }
}

void KernelConsole::DoParse() {
  auto input = Split(std::string_view(input_buffer_, input_buffer_size_), ' ');

  // Clear the input buffer.
  input_buffer_size_ = 0;

  if (input.size() == 0) {
    return;
  }

  // Handle special commands here first.
  if (input[0] == "cd") {
    QemuSerialLog::Logf("cd is called!");
    return;
  } else if (input[0] == "clear") {
    VGAOutput::GetVGAOutput().ClearScreen();
    should_show_shell_prefix_ = true;
    return;
  } else if (input[0] == "ps") {
    const auto& th_per_core =
        KernelThreadScheduler::GetKernelThreadScheduler().NumThreadsPerCore();
    for (size_t i = 0; i < th_per_core.size(); i++) {
      kprintf("CPU %d has [%d] thread(s) \n", i, th_per_core.at(i));
    }
    kprintf("\n");
    return;
  }

  std::vector<KernelString> argv;
  for (auto s : input) {
    argv.push_back(s);
  }

  fg_process_ =
      ProcessManager::GetProcessManager().CreateProcess(input[0], argv);
  if (fg_process_ == nullptr) {
    kprintf("%s is not found. \n", KernelString(input[0]).c_str());
    return;
  }

  fg_process_->Start();

  // Wait until it finishes.
  fg_process_->Join();

  fg_process_ = nullptr;
}

void KernelConsole::PrintToTerminal(char* data, int sz) {
  for (int i = 0; i < sz; i++) {
    size_t write_index =
        __atomic_fetch_add(&term_output_write_index_, 1, __ATOMIC_RELAXED);
    term_output_buffer_[write_index % kTerminalOutputBufferSize] = data[i];
  }
}

void KernelConsole::PrintTermOutputBuffer() {
  while (true) {
    size_t write_index = term_output_write_index_ % kTerminalOutputBufferSize;
    if (write_index == term_output_read_index_) {
      KernelThreadScheduler::GetKernelThreadScheduler().Yield();
      continue;
    }

    int num_to_read = 0;
    if (write_index > term_output_read_index_) {
      num_to_read = write_index - term_output_read_index_;
    } else {
      num_to_read =
          term_output_read_index_ - write_index + kTerminalOutputBufferSize;
    }

    for (int i = 0; i < num_to_read; i++) {
      if (i + term_output_read_index_ < kTerminalOutputBufferSize) {
        term_output_buffer_to_print_[i] =
            term_output_buffer_[i + term_output_read_index_];
      } else {
        term_output_buffer_to_print_[i] =
            term_output_buffer_[i + term_output_read_index_ -
                                kTerminalOutputBufferSize];
      }
    }

    // Add NULL terminator.
    term_output_buffer_to_print_[num_to_read] = 0;

    term_output_read_index_ += num_to_read;
    if (term_output_read_index_ >= kTerminalOutputBufferSize) {
      term_output_read_index_ -= kTerminalOutputBufferSize;
    }

    kprintf("%s", term_output_buffer_to_print_);
  }
}

void KernelConsole::ShowWelcome() {
  std::string_view welcome = R"(
              a8888b.
             d888888b.
             8P"YP"Y88
             8|o||o|88   Welcome To
             8'    .88    ModooOs
             8`._.' Y8.
            d/      `8b.
           dP   .    Y8b.
          d8:'  "  `::88b
         d8"         'Y88b
        :8P    '      :888
         8a.   :     _a88P
       ._/"Yaa_:   .| 88P|
  jgs  \    YP"    `| 8P  `.
  a:f  /     \.___.d|    .'
       `--..__)8888P`._.'
)";

  VGAOutput::GetVGAOutput().PrintString(welcome, VGAColor::Green);
}

}  // namespace Kernel
