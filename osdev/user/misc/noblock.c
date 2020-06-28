#include <printf.h>
#include <syscall.h>

int main() {
  console(SET_NO_BUFFER);

  while (1) {
    char buf[100];
    int cnt = read(0, buf, 100);
    buf[cnt] = 0;

    printf("You wrote: %s\n", buf);
    if (buf[0] == 'q') {
      break;
    }
  }

  return 0;
}
