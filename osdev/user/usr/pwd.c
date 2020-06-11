#include <printf.h>
#include <syscall.h>

int main() {
  char current_dir[1024];
  char* buf = getcwd(current_dir, 1024);

  if (buf == 0) {
    sprintf(current_dir,
            "Current directory name is too long (>= 1024) Unable to print.");
    write(current_dir);
  } else {
    write(current_dir);
    write("\n");
  }

  return 0;
}

