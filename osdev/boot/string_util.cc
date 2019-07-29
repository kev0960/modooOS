#include "string_util.h"

namespace Kernel {
unsigned int strlen(const char* s) {
  unsigned int len = 0;
  while (*s++) {
    len++;
  }
  return len;
}
}  // namespace Kernel

