#include <stdlib.h>

int atoi(const char *str) {
  int num = 0;
  while (*str) {
    num *= 10;
    num += (*str - '0');
    str ++;
  }
  return num;
}
