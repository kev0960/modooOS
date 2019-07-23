#ifndef STRING_UTIL_H
#define STRING_UTIL_H

namespace Kernel {
unsigned int strlen(const char* s) {
  unsigned int len = 0;
  while (*s++) {
    len++;
  }
  return len;
}
}  // namespace Kernel

#endif
