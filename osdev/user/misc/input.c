#include <printf.h>
#include <syscall.h>

int main() {
  float a = 3.1;
  for (int i = 0; i < 10; i++) {
    printf("Float : %f \n", a);
    a = a * 1.7;
  }

  for (int i = 0; i < 4; i++) {
    char buf[100];
    int cnt = read(0, buf, 100);
    buf[cnt] = 0;

    printf("You wrote: %s\n", buf);
  }

  return 0;
}
