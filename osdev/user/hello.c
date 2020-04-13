#include "sha1.h"

typedef uint64_t size_t;

size_t strlen(const char* s) {
  size_t sz = 0;
  while (*s != 0) {
    sz++;
    s++;
  }
  return sz;
}

void write(const char* s) {
  size_t count = strlen(s);
  asm volatile(
      "push %%rbx\n"
      "push %%r12\n"
      "push %%r13\n"
      "push %%r14\n"
      "push %%r15\n"
      "mov %1, %%rdx\n"
      "mov %0, %%rsi\n"
      "mov $1, %%rdi\n"  // STDOUT
      "mov $2, %%rax\n"
      "syscall\n"
      "pop %%r15\n"
      "pop %%r14\n"
      "pop %%r13\n"
      "pop %%r12\n"
      "pop %%rbx\n" ::"r"(s),
      "r"(count)
      :);
}

int main() {
  for (int i = 0; i < 10; i++) {
    write("Hello World this is printed from the process!\n");
  }
  return 0;
}
