int main() {
  while (1) {
    asm volatile(
        "push %%rbx\n"
        "push %%r12\n"
        "mov $7, %%r9 \n"
        "mov $6, %%r8 \n"
        "mov $5, %%r10\n"
        "mov $4, %%rdx \n"
        "mov $3, %%rsi\n"
        "mov $2, %%rdi\n"
        "mov $1, %%rax \n"
        "syscall\n"
        "pop %%r12\n"
        "pop %%rbx\n" ::
            : "rax");
  }
  return 0;
}
