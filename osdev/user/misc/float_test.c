#include <stdlib.h>
#include <stdio.h>
#include <syscall.h>

int main(int argc, char* argv[]) {
  if (argc < 2) {
    printf("Please specify the argument!");
    return 0;
  }

  for (int i = 0; i < argc - 1; i++) {
    printf("Converting [%s] --> [%lf] \n", argv[i + 1], atof(argv[i + 1]));
  }

  return 0;
}
