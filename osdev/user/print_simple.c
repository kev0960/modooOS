#include "./libc/printf.h"
#include "./libc/syscall.h"

int main() {
  write("Hello, world!\n");

  int fd = open("/temp.txt");
  char str_out[30];
  sprintf(str_out, "fd:%d\n", fd);
  write(str_out);

  char buf[30];
  sprintf(str_out, "buf:%lx\n", buf);
  write(str_out);

  read(fd, buf, 29);
  buf[29] = '\0';

  write(buf);

  char buf2[100];
  int fd2 = open("/temp");
  read(fd2, buf2, 99);
  buf2[99] = 0;
  write(buf2);

  return 123;
}
