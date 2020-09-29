#include <printf.h>
#include <stdlib.h>
#include <syscall.h>

int main(int argc, char* argv[]) {
  int sleep_ms = 1000;

  if (argc >= 2) {
    sleep_ms = atoi(argv[1]);
  }
  printf("Sleep : %d \n", sleep_ms);

  while (1) {
    size_t cur = mstick();
    usleep(1000 * sleep_ms);
    size_t aft = mstick();
    printf("cur : %d aft : %d diff : %d\n", cur, aft, aft - cur);
  }
}
