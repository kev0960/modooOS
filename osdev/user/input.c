#include "./libc/printf.h"
#include "./libc/syscall.h"

int main() {
  for (int i = 0; i < 4; i++) {
    char buf[100];
    int cnt = read(0, buf, 100);
    buf[cnt] = 0;

    char out[200];
    sprintf(out, "You wrote: %s\n", buf);
    write(out);
  }

  return 0;
}
