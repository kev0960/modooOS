#include <printf.h>
#include <syscall.h>

uint32_t read_4bytes(char* buf) { return *(uint32_t*)(buf); }
uint16_t read_2bytes(char* buf) { return *(uint16_t*)(buf); }
uint8_t read_1bytes(char* buf) { return *(uint8_t*)(buf); }

int main(int argc, char* argv[]) {
  (void)argc;
  (void)argv;

  char current_dir[100];
  getcwd(current_dir, 100);

  int fd = open(current_dir, 0);

  char dir_info[1024];
  int cnt = get_dents(fd, (struct linux_dirent*)dir_info, 1024);

  char* current = dir_info;
  while (current - dir_info < cnt) {
    // Ignore inode num.
    current += 4;

    // Read total entry size.
    uint16_t d_off = read_2bytes(current);
    current += 2;

    if (d_off == 0) {
      break;
    }

    // Read name length
    uint8_t name_len = read_1bytes(current);
    current += 1;

    // Read type indicator
    current += 1;

    char file_name[256];
    for (int i = 0; i < name_len; i++) {
      file_name[i] = current[i];
    }
    file_name[name_len] = 0;

    // Now it contains d_name, which includes the Null terminator.
    printf("%s", file_name);
    printf(" ");

    current -= 8;
    current += d_off;
  }

  printf("\n");

  return 0;
}
