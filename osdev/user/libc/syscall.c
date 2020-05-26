#include "syscall.h"

int64_t syscall_1(int64_t sysnum, int64_t arg1) {
  int64_t ret;
  asm volatile(
      "push %%rbx\n"
      "push %%r12\n"
      "push %%r13\n"
      "push %%r14\n"
      "push %%r15\n"
      "syscall\n"
      "pop %%r15\n"
      "pop %%r14\n"
      "pop %%r13\n"
      "pop %%r12\n"
      "pop %%rbx\n"
      : "=a"(ret)
      : "a"(sysnum), "D"(arg1)
      :);
  return ret;
}

int64_t syscall_2(int64_t sysnum, int64_t arg1, int64_t arg2) {
  int64_t ret;
  asm volatile(
      "push %%rbx\n"
      "push %%r12\n"
      "push %%r13\n"
      "push %%r14\n"
      "push %%r15\n"
      "syscall\n"
      "pop %%r15\n"
      "pop %%r14\n"
      "pop %%r13\n"
      "pop %%r12\n"
      "pop %%rbx\n"
      : "=a"(ret)
      : "a"(sysnum), "D"(arg1), "S"(arg2)
      :);
  return ret;
}

int64_t syscall_3(int64_t sysnum, int64_t arg1, int64_t arg2, int64_t arg3) {
  int64_t ret;
  asm volatile(
      "push %%rbx\n"
      "push %%r12\n"
      "push %%r13\n"
      "push %%r14\n"
      "push %%r15\n"
      "syscall\n"
      "pop %%r15\n"
      "pop %%r14\n"
      "pop %%r13\n"
      "pop %%r12\n"
      "pop %%rbx\n"
      : "=a"(ret)
      : "a"(sysnum), "D"(arg1), "S"(arg2), "d"(arg3)
      :);
  return ret;
}

size_t strlen(const char* s) {
  size_t sz = 0;
  while (*s != 0) {
    sz++;
    s++;
  }
  return sz;
}

int open(const char* pathname) { return syscall_1(7, (int64_t)pathname); }

size_t read(int64_t fd, char* buf, size_t count) {
  return syscall_3(1, fd, (int64_t)buf, count);
}

size_t write(const char* s) {
  size_t count = strlen(s);
  return syscall_3(2, /*stdout*/ 1, (int64_t)s, count);
}

int spawn(pid_t* pid, const char* s) {
  return syscall_2(5, (int64_t)pid, (int64_t)s);
}

pid_t waitpid(pid_t pid, int* status) {
  return syscall_2(6, pid, (int64_t)status);
}

