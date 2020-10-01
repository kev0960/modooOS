#ifndef LIBC_STDLIB_H
#define LIBC_STDLIB_H

#include <malloc.h>

int abs(int n);
long labs(long n);

int atoi(const char* str);
double atof(const char* s);

// Do nothing.
void exit(int exit_code);

#endif
