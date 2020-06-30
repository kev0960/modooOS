#include "string.h"

#include <stdint.h>
#include <stdlib.h>

void* memcpy(void* dest, const void* src, size_t count) {
  const char* s = src;
  char* d = dest;
  while (count-- > 0) {
    *d = *s;
    d++;
    s++;
  }
  return dest;
}

void* memset(void* dest, int ch, size_t count) {
  char* d = dest;
  while (count-- > 0) {
    *d = ch;
    d++;
  }
  return dest;
}

void* memmove(void* dest, const void* src, size_t count) {
  size_t srcp = (size_t)src;
  size_t destp = (size_t)dest;

  // They do not overlap.
  if (destp > srcp && destp - srcp >= count) {
    return memcpy(dest, src, count);
  } else if (srcp > destp && srcp - destp >= count) {
    return memcpy(dest, src, count);
  }

  char* mem = malloc(count);
  memcpy(mem, src, count);
  dest = memcpy(dest, mem, count);
  free(mem);

  return dest;
}

int memcmp(const void* lhs, const void* rhs, size_t count) {
  const char* lhs_p = lhs;
  const char* rhs_p = rhs;

  while (count--) {
    if (*lhs_p != *rhs_p) {
      return *lhs_p - *rhs_p;
    }

    lhs_p++;
    rhs_p++;
  }

  return 0;
}

int strcmp(const char* lhs, const char* rhs) {
  while (*lhs && (*lhs == *rhs)) {
    lhs++;
    rhs++;
  }

  int lc = *lhs;
  int rc = *rhs;

  return lc - rc;
}

int strncmp(const char* lhs, const char* rhs, size_t count) {
  while (count--) {
    if (*lhs != *rhs) {
      return *(unsigned char*)(lhs) - *(unsigned char*)(rhs);
    }
    lhs++;
    rhs++;
  }
  return 0;
}

size_t strlen(const char* str) {
  size_t sz = 0;
  while (*str) {
    str++;
    sz++;
  }
  return sz;
}

char* strchr(const char* str, int ch) {
  while (*str != (char)ch) {
    if (!*str++) {
      return 0;
    }
  }

  return (char*)str;
}

int strcasecmp(const char* lhs, const char* rhs) {
  while (*lhs && (*lhs == *rhs)) {
    lhs++;
    rhs++;
  }

  int lc = *lhs;
  int rc = *rhs;

  return lc - rc;
}

int strncasecmp(const char* lhs, const char* rhs, size_t count) {
  while (count--) {
    if (*lhs != *rhs) {
      return *(unsigned char*)(lhs) - *(unsigned char*)(rhs);
    }
    lhs++;
    rhs++;
  }
  return 0;
}

int utf8_str_to_unicode_num(const char* str, int* unicode) {
  *unicode = 0;

  if ((uint8_t)str[0] <= 0x7F) {
    *unicode = str[0];
    return 1;
  } else if ((uint8_t)str[0] <= 0xDF) {
    *unicode = str[1] & (0b00111111);
    *unicode |= ((int)(str[0] & (0b00011111)) << 6);
    return 2;
  } else if ((uint8_t)str[0] <= 0b11101111) {
    *unicode = str[2] & (0b00111111);
    *unicode |= ((int)(str[1] & 0b00111111) << 6);
    *unicode |= ((int)(str[0] & 0b00001111) << 12);
    return 3;
  } else if ((uint8_t)str[0] <= 0b11110111) {
    *unicode = str[3] & (0b00111111);
    *unicode |= ((int)(str[2] & 0b00111111) << 6);
    *unicode |= ((int)(str[1] & 0b00111111) << 12);
    *unicode |= ((int)(str[0] & 0b00000111) << 18);
    return 4;
  }

  return 0;
}
