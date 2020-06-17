#include <printf.h>
#include <syscall.h>

int main() {
  char current_dir[1024];
  char* buf = getcwd(current_dir, 1024);

  if (buf == 0) {
    printf("Current directory name is too long (>= 1024) Unable to print.");
  } else {
    printf("%s", current_dir);
    printf("\n");
  }

  return 0;
}

