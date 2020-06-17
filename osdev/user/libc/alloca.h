#ifndef LIBC_ALLOCA_H
#define LIBC_ALLOCA_H

#include <stddef.h>

void* alloca(size_t size);
#define alloca(size) __builtin_alloca(size)

#endif
