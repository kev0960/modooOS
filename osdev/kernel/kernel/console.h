#ifndef CONSOLE_H
#define CONSOLE_H

#include "../std/string.h"

namespace Kernel {

// Creates a kernel console. Kernel console handles following things:
//  - Runs a shell (/bin/bash)
//  - When the user types, print it to the console
//     - Kernel console receives direct input from the keyboard driver.
//  - When the user finishes input, send it to the shell.
class KernelConsole {
 public:
};

}  // namespace Kernel

#endif
