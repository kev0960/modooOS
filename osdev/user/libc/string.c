#include "string.h"

void* memcpy(void* dest, const void* src, size_t count) {
  const char* s = src;
  char* d = dest;
  while (count-- > 0) {
    *d = *s;
    d++;
    s++;
  }
  return d;
}

void* memset(void* dest, int ch, size_t count) {
  char* d = dest;
  while (count-- > 0) {
    *d = ch;
    d++;
  }
  return d;
}
