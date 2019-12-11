#include "string_util.h"

namespace Kernel {
unsigned int strlen(const char* s) {
  unsigned int len = 0;
  while (*s++) {
    len++;
  }
  return len;
}

int strcmp(const char* s, const char* t) {
  char c1, c2;

  do {
    c1 = *s++;
    c2 = *t++;

    if (c1 == 0) {
      return c1 - c2;
    }
  } while (c1 == c2);

  return c1 - c2;
}

}  // namespace Kernel

