#include <stdio.h>

int main() {
  FILE* f = fopen("/misc/temp", "w");
  fprintf(f, "hello");

  return 0;
}
