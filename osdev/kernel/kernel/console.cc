#include "console.h"

#include "../std/string_view.h"
#include "./fs/ext2.h"
#include "graphic.h"
#include "process.h"
#include "scheduler.h"
#include "string.h"
#include "utf8.h"
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

  // This thread will wait for the foreground process to join.
  KernelThread* wait_fg_process_thread = new KernelThread([] {
    auto& console = KernelConsole::GetKernelConsole();
    bool process_just_joined = false;
    while (true) {
      Process* fg_process = console.GetForegroundProcess();
      if (fg_process) {
        fg_process->Join();
        console.SetForegroundProcess(nullptr);
        process_just_joined = true;
      }
      KernelThreadScheduler::GetKernelThreadScheduler().Yield();

      if (fg_process) {
        QemuSerialLog::Logf("Deleting FG\n");
        delete fg_process;
      }

      if (process_just_joined) {
        console.ShowShellPrefix();
        process_just_joined = false;
      }
    }
  });

  wait_fg_process_thread->Start();
}

KernelConsole::KernelConsole()
    : console_pipe_(new Pipe()),
      console_pipe_read_end_(console_pipe_),
      console_pipe_write_end_(console_pipe_),
      working_dir_("/") {
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
  fg_process_started_ = false;
}

void KernelConsole::AddKeyStroke(const KeyStroke& key) {
  // Handle some commands that requires immediate intervertion (e.g ctrl-c to
  // kill the current running process.)
  if (key.c == 'c' && key.is_ctrl_down) {
    if (fg_process_ != nullptr) {
      fg_process_->MakeTerminateReady();
    }
  }

  // We only record "Up" only when RECORD_UP is enabled.
  if (current_write_index_ == kKeyStrokeBufferSize) {
    current_write_index_ = 0;
  }

  if (key.action == KEY_UP && !(keystroke_mode_ & RECORD_UP)) {
    return;
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

  QemuSerialLog::Logf("STart Kernel console!");

  while (true) {
    if (should_show_shell_prefix_) {
      char prefix[1024];
      sprintf(prefix, "root:%s# ", working_dir_.c_str());
      GraphicManager::GetGraphicManager().PrintAnchorString(prefix);
      should_show_shell_prefix_ = false;
    }

    int num_received = ReadKeyStroke(&received_keyinfo_queue);
    // Yield if nothing is received.
    if (num_received == 0) {
      KernelThreadScheduler::GetKernelThreadScheduler().Yield();
      continue;
    }

    // Print key strokes to the console.
    GraphicManager::GetGraphicManager().PrintKeyStrokes(received_keyinfo_queue,
                                                        0, num_received);

    // Fill buffer and parse if needed.
    FillInputBufferAndParse(num_received);
  }
}

void KernelConsole::FillInputBufferAndParse(int num_received) {
  for (int i = 0; i < num_received; i++) {
    // Note that we do not handle input buffer until the user hits the ENTER.
    if (received_keyinfo_queue[i].c == KEY_CODES::ENTER) {
      if (fg_process_ == nullptr) {
        fg_process_started_ = false;
        // Parse the input.
        DoParse();

        if (!fg_process_started_) {
          should_show_shell_prefix_ = true;
        }
      } else {
        // Need to include ENTER.
        input_buffer_[input_buffer_size_] = '\r';
        input_buffer_size_++;

        SendInputBufferToFgProcess(input_buffer_size_);
      }
    } else if (received_keyinfo_queue[i].c == KEY_CODES::BACKSPACE) {
      if (input_buffer_size_ > 0) {
        input_buffer_size_--;
      }
    } else {
      char c = received_keyinfo_queue[i].ToChar();

      // KEY status is recorded as a prefix.
      if (keystroke_mode_ & RECORD_UP) {
        if (received_keyinfo_queue[i].action == KEY_UP) {
          input_buffer_[input_buffer_size_++] = 1;
        } else if (received_keyinfo_queue[i].action == KEY_DOWN) {
          input_buffer_[input_buffer_size_++] = 2;
        }
      }

      if (c != 0) {
        input_buffer_[input_buffer_size_] = c;
        input_buffer_size_++;
      } else if (keystroke_mode_ & EVERYTHING) {
        // Even if 'c' is not ASCII character, we should keep in in EVERYTHING
        // mode. Here, we convert c using UTF-8 encoding.
        // TODO Keystorke code actually does not match real UNICODE value.
        // Figure out the way to actually send the scan code.
        input_buffer_size_ += SetUnicodeToUTF8(
            received_keyinfo_queue[i].c, input_buffer_ + input_buffer_size_);
      }

      // If no buffering is enabled, then we immediately send input keystroke to
      // the running process.
      if (fg_process_ != nullptr && buffering_mode_ == NO_BUFFER) {
        SendInputBufferToFgProcess(input_buffer_size_);
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
    if (input.size() > 1) {
      DoCd(input[1]);
    } else {
      kprintf("Please specify a directory \n");
    }
    return;
  } else if (input[0] == "clear") {
    VGAOutput::GetVGAOutput().ClearScreen();
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

  // Set the bufering mode for this new process as a line buffer.
  // (Inputs will not be passed to the user process until the ENTER has hit).
  buffering_mode_ = LINE_BUFFER;
  keystroke_mode_ = ASCII_ONLY;

  // Default is blocking IO.
  console_pipe_->SetBlocking(true);

  auto& ext2 = Ext2FileSystem::GetExt2FileSystem();
  auto file_path = ext2.GetCanonicalAbsolutePath(
      ext2.GetAbsolutePath(input[0], working_dir_));

  fg_process_ = ProcessManager::GetProcessManager().CreateProcess(
      file_path.c_str(), working_dir_.c_str(), argv);
  if (fg_process_ == nullptr) {
    // Try again with input[0] with /usr/bin path.
    // TODO Later we have to introduce env variables.
    auto file_path = ext2.GetAbsolutePath(input[0], "/usr/bin");
    fg_process_ = ProcessManager::GetProcessManager().CreateProcess(
        file_path.c_str(), working_dir_.c_str(), argv);

    if (fg_process_ == nullptr) {
      kprintf("%s is not found. \n", KernelString(input[0]).c_str());
      return;
    }
  }

  // Associate fore ground process's output buffer to console's pipe.
  FileDescriptorTable& table = fg_process_->GetFileDescriptorTable();
  table.SetDescriptor(FileDescriptorTable::STDIO::STDIN,
                      &console_pipe_read_end_);

  fg_process_->Start();
  fg_process_started_ = true;

  // By yielding here, print_terminal_thread will be enqueued. This will give a
  // chance to flush every remaining output buffer before printing the prompt
  // back.
  KernelThreadScheduler::GetKernelThreadScheduler().Yield();
}

void KernelConsole::SendInputBufferToFgProcess(int index) {
  // Send [0, index) characters in the input buffer to the foreground process.
  console_pipe_write_end_.Write(input_buffer_, index);
  input_buffer_size_ = 0;
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

    /*
    VGAOutput::GetVGAOutput().PrintLock();
    for (int i = 0; i < num_to_read; i++) {
      VGAOutput::GetVGAOutput().PutCharWithoutLock(
          term_output_buffer_to_print_[i]);
    }
    VGAOutput::GetVGAOutput().PrintUnlock();
    */

    auto& gm = GraphicManager::GetGraphicManager();
    gm.PrintLock();
    for (int i = 0; i < num_to_read; i++) {
      gm.PutChar(term_output_buffer_to_print_[i]);
    }
    gm.PrintUnlock();
  }
}

void KernelConsole::DoCd(const KernelString& path) {
  auto& ext2 = Ext2FileSystem::GetExt2FileSystem();
  auto new_working_dir =
      ext2.GetCanonicalAbsolutePath(ext2.GetAbsolutePath(path, working_dir_));

  // Check that new working dir is a directory.
  FileInfo file_info = ext2.Stat(new_working_dir.c_str());
  if (Ext2FileSystem::GetFileFormatFromMode(file_info.mode) !=
      Ext2FileSystem::S_DIR) {
    kprintf("cd: '%s' does not exist or not a directory.\n", path.c_str());
    return;
  }

  working_dir_ = new_working_dir;
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
  jae \    YP"    `| 8P  `.
  bum  /     \.___.d|    .'
       `--..__)8888P`._.'
)";

  GraphicManager::GetGraphicManager().PrintString(welcome, 0x39ff14);
}

}  // namespace Kernel
