#include "./libc/printf.h"
#include "./libc/syscall.h"

int main(int argc, char* argv[]) {
  char str_out[100];
  sprintf(str_out, "Argc : %d argv %lx *argv: %lx\n", argc, argv, *argv);
  write(str_out);

  for (int i = 0; i < argc; i++) {
    sprintf(str_out, "argv : [%lx] '%s' \n", argv[i], argv[i]);
    write(str_out);
  }

  return 0;
}
