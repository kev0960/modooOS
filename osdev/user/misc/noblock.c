#include <printf.h>
#include <string.h>
#include <syscall.h>

int main() {
  console(SET_NO_BUFFER | SET_EVERYTHING | SET_RECORD_UP);

  while (1) {
    char buf[100];
    int cnt = read(0, buf, 100);
    if (cnt == 0) {
      continue;
    }
    buf[cnt] = 0;

    if (buf[0] == 1) {
      printf("Up ");
    } else if (buf[0] == 2) {
      printf("Down ");
    }

    int unicode;
    int len = utf8_str_to_unicode_num(&buf[1], &unicode);
    if (cnt >= 3) {
      printf("%x %x", (int)buf[1], (int)buf[2]);
    }
    printf("You wrote: %x [len : %d] %d\n", unicode, len, cnt);

    if (buf[0] == 'q') {
      break;
    }
  }

  return 0;
}
