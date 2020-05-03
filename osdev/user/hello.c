#include "./libc/printf.h"
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
  /*
  //const char* s = "abc";
  for (int i = 0; i < 10; i++) {
    write("Hello World this is printed from the process!\n");
  }
  */
  uint8_t output[20];

  SHA1_CTX ctx;
  SHA1Init(&ctx);

  const char* buf = "abc";
  SHA1Update(&ctx, (uint8_t*)buf, 3);
  SHA1Final(output, &ctx);

  char str_out[41];
  for (int i = 0; i < 20; i++) {
    sprintf(&str_out[2 * i], "%02x", output[i]);
  }
  str_out[40] = '\0';
  write(str_out);
  return 0;
}
