#include <ctype.h>
#include <stdlib.h>

int abs(int n) {
  if (n < 0) {
    return -n;
  }

  return n;
}

long labs(long n) {
  if (n < 0) {
    return -n;
  }

  return n;
}

int atoi(const char* str) {
  int num = 0;
  while (*str) {
    num *= 10;
    num += (*str - '0');
    str++;
  }
  return num;
}

// Shamelessly copied from
// https://github.com/GaloisInc/minlibc/blob/master/atof.c
double atof(const char* s) {
  double a = 0.0;
  int e = 0;
  int c;
  int a_sign = 1;

  // Ignore the preceding spaces.
  while (*s != 0 && isspace(*s)) {
    s++;
  }

  if (*s == '-') {
    a_sign = -1;
    s++;
  } else if (*s == '+') {
    s++;
  }

  while ((c = *s++) != '\0' && isdigit(c)) {
    a = a * 10.0 + (c - '0');
  }
  if (c == '.') {
    while ((c = *s++) != '\0' && isdigit(c)) {
      a = a * 10.0 + (c - '0');
      e = e - 1;
    }
  }
  if (c == 'e' || c == 'E') {
    int sign = 1;
    int i = 0;
    c = *s++;
    if (c == '+')
      c = *s++;
    else if (c == '-') {
      c = *s++;
      sign = -1;
    }
    while (isdigit(c)) {
      i = i * 10 + (c - '0');
      c = *s++;
    }
    e += i * sign;
  }
  while (e > 0) {
    a *= 10.0;
    e--;
  }
  while (e < 0) {
    a *= 0.1;
    e++;
  }
  return a * a_sign;
}

void exit(int exit_code) { (void)exit_code; }
