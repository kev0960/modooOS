#include <printf.h>
#include <syscall.h>

int main(int argc, char* argv[]) {
  char str_out[100];

  if (argc < 2) {
    sprintf(str_out, "Please specify file to print.");
    write(str_out);
    return 0;
  }

  sprintf(str_out, "Reading %s \n", argv[1]);
  write(str_out);

  struct stat file_info;
  int ret = stat(argv[1], &file_info);

  if (ret == -1) {
    sprintf(str_out, "File %s is not found!\n", argv[1]);
    write(str_out);
    return 0;
  }

  sprintf(str_out, "File %s size : %d.\n", argv[1], file_info.file_size);
  write(str_out);

  int fd = open(argv[1]);
  char buf[1025];
  size_t cnt = 0;
  while (cnt < file_info.file_size) {
    int read_num = read(fd, buf, 1024);
    buf[read_num] = 0;
    write(buf);
    cnt += read_num;
  }

  return 0;
}
