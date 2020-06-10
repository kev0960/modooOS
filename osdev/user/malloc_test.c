#include "./libc/printf.h"
#include "./libc/syscall.h"

int main() {
  int* arr = (int*)sbrk(1000);
  for (int i = 0; i < 30; i++) {
    arr[i] = i;
  }

  char data[100];
  for (int i = 0; i < 30; i++) {
    sprintf(data, "arr : %d with addr : %p \n", arr[i], &arr[i]);
    write(data);
  }

  return 0;
}
