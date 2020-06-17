#include <printf.h>
#include <syscall.h>

int main(int argc, char* argv[]) {
  if (argc < 2) {
    printf("Please specify file to print.");
    return 0;
  }

  printf("Reading %s \n", argv[1]);

  struct stat file_info;
  int ret = stat(argv[1], &file_info);

  if (ret == -1) {
    printf("File %s is not found!\n", argv[1]);
    return 0;
  }

  printf("File %s size : %d.\n", argv[1], file_info.file_size);

  int fd = open(argv[1]);
  char buf[1025];
  size_t cnt = 0;
  while (cnt < file_info.file_size) {
    int read_num = read(fd, buf, 1024);
    buf[read_num] = 0;
    printf("%s", buf);
    cnt += read_num;
  }

  return 0;
}
