#ifndef CONSOLE_H
#define CONSOLE_H

#include "../std/string.h"
#include "keyboard.h"
#include "process.h"

namespace Kernel {

// Creates a kernel console. Kernel console handles following things:
//  - Runs a shell (/bin/bash)
//  - When the user types, print it to the console
//     - Kernel console receives direct input from the keyboard driver.
//     - Decide what to return to the running process.
//  - When the user finishes input, send it to the shell.
class KernelConsole {
 public:
  // Start the kernel console.
  static void InitKernelConsole();

  static KernelConsole& GetKernelConsole() {
    static KernelConsole console;
    return console;
  }

  void Run();
  bool IsRunning() const { return is_running_; }

  // Add key stroke (called by Keyboard Handler).
  void AddKeyStroke(const KeyStroke& key);

  // Read the received keystorkes. (Non-block)
  // NOTE: This will not properly return the received keystrokes if the user
  // happens to send more than kKeyStrokeBufferSize strokes in between call of
  // this function.
  // NOTE: It is possible that AddKeyStroke can be called while executing this
  // function.
  int ReadKeyStroke(std::vector<KeyStroke>* strokes);

  void PrintToTerminal(char* data, int sz);
  void PrintTermOutputBuffer();

  void ShowWelcome();

 private:
  static constexpr int kInputLineBufferSize = 1024;
  static constexpr int kKeyStrokeBufferSize = 1024;

  // 1 MB of the terminal output buffer.
  static constexpr int kTerminalOutputBufferSize = 1048576 - 1;

  KernelConsole();

  void FillInputBufferAndParse(int num_received);
  void DoParse();

  // Current foreground process.
  Process* fg_process_;

  // List of background processes.
  std::vector<Process*> bg_process_list_;

  // This is the lock-free version of the list of the received key strokes from
  // the keyboard.
  std::vector<KeyStroke> received_keyinfo_queue;
  size_t current_read_index_;
  size_t current_write_index_;

  bool is_running_;

  // This is the input buffer exclusively used for the input.
  char* input_buffer_;
  int input_buffer_size_;

  // If true, print fancy shell prefix ("root$").
  bool should_show_shell_prefix_;

  // Anything that the process prints with "write" syscall to the stdout will be
  // written here.
  char* term_output_buffer_;
  size_t term_output_read_index_;
  size_t term_output_write_index_;

  char* term_output_buffer_to_print_;
};

}  // namespace Kernel

#endif
