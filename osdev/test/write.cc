#include "printf.h"
#include "syscall.h"

typedef unsigned long int uintptr; /* size_t */
typedef long int intptr;           /* ssize_t */

static intptr write(int fd, void const *data, uintptr nbytes) {
  return (intptr)_syscall5((void *)1, /* SYS_write */
                           (void *)(intptr)fd, (void *)data, (void *)nbytes,
                           0, /* ignored */
                           0  /* ignored */
  );
}


void _putchar(char character) {
  write(/* STDOUT */ 1, &character, 1);
}
