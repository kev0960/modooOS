#ifndef STRING_H
#define STRING_H
#include "types.h"

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

}  // namespace Kernel
#endif
