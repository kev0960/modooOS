#ifndef STRING_UTIL_H
#define STRING_UTIL_H

#include "algorithm.h"
#include "types.h"

#define NTOA_BUFFER_SIZE 20

namespace Kernel {
unsigned int strlen(const char* s);

template <typename Int>
void ntoa(char* str, size_t size, Int num, size_t base = 10) {
  char buffer[NTOA_BUFFER_SIZE];
  size_t num_len = 0;

  bool is_negative = (num < 0);
  if (is_negative) {
    num = -num;
  }

  for (size_t i = 0; i < size; i++) {
    num_len++;

    buffer[i] = num % base;
    num /= base;
    if (num == 0) {
      break;
    }
  }

  if (num != 0) {
    str[0] = 'E';
    str[1] = 0;
    return;
  }

  size_t offset = 0;
  if (is_negative) {
    str[offset++] = '-';
  }
  if (base == 8) {
    str[offset++] = '0';
  } else if (base == 16) {
    str[offset++] = '0';
    str[offset++] = 'x';
  } else if (base == 2) {
    str[offset++] = '0';
    str[offset++] = 'b';
  }

  for (size_t i = 0; i < num_len; i++) {
    if (buffer[num_len - i - 1] >= 10) {
      str[i + offset] = 'A' + buffer[num_len - i - 1] - 10;
    } else {
      str[i + offset] = '0' + buffer[num_len - i - 1];
    }
  }
  str[num_len + offset] = 0;
}
}  // namespace Kernel

#endif
