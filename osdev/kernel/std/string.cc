#include "string.h"

namespace Kernel {

void* memcpy(void* dest, void* src, size_t count) {
  char* s = reinterpret_cast<char*>(src);
  char* d = reinterpret_cast<char*>(dest);
  while (count-- > 0) {
    *d = *s;
    d++;
    s++;
  }
  return d;
}

void* memset(void* dest, int ch, size_t count) {
  char* d = reinterpret_cast<char*>(dest);
  while (count-- > 0) {
    *d = ch;
    d++;
  }
  return d;
}

int strncmp(const char* lhs, const char* rhs, size_t count) {
  for (size_t i = 0; i < count; i++) {
    if (lhs[i] == rhs[i]) {
      continue;
    }
    return lhs[i] - rhs[i];
  }
  return 0;
}

}  // namespace Kernel

