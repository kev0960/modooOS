#include <printf.h>
#include <syscall.h>

int main() {
  uint64_t rbp;
  asm volatile("mov %%rbp, %0\n" : "=m"(rbp)::);
  printf("rbp:%lx", rbp);

  // Now Write to stdout is identical to write to pipe.
  dup2(4, 1);

  printf("Hello, world!\n");

  int fd = open("/temp.txt");
  printf("fd:%d\n", fd);

  char buf[30];
  printf("buf:%lx\n", buf);

  read(fd, buf, 29);
  buf[29] = '\0';

  printf("%s", buf);

  char buf2[100];
  int fd2 = open("/temp.txt");
  read(fd2, buf2, 99);
  buf2[99] = 0;
  printf("%s", buf2);
  return 123;
}
