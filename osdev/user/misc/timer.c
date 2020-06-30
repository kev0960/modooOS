#include <printf.h>
#include <syscall.h>

int main() {
  while (1) {
    size_t cur = mstick();
    usleep(1000*1000);
    size_t aft = mstick();
    printf("cur : %d aft : %d \n", cur, aft);
    printf("diff : %d", aft - cur);
  }
}
