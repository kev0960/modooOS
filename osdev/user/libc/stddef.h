#ifndef LIBC_STDDEF_H
#define LIBC_STDDEF_H

#include <stdbool.h>
#include <stdint.h>

#define NULL (0)

typedef uint64_t size_t;
typedef int64_t ptrdiff_t;

#define offsetof(t, d) __builtin_offsetof(t, d)

#endif
