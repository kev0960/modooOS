#include "syscall.h"

#include <string.h>

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

int open(const char* pathname) { return syscall_1(7, (int64_t)pathname); }

size_t read(int64_t fd, char* buf, size_t count) {
  return syscall_3(1, fd, (int64_t)buf, count);
}

size_t write(int fd, const char* s, size_t count) {
  return syscall_3(2, /*stdout*/ fd, (int64_t)s, count);
}

int spawn(pid_t* pid, const char* s) {
  return syscall_2(5, (int64_t)pid, (int64_t)s);
}

pid_t waitpid(pid_t pid, int* status) {
  return syscall_2(6, pid, (int64_t)status);
}

int pipe(int pipefd[2]) { return syscall_1(8, (int64_t)pipefd); }
int dup2(int oldfd, int newfd) { return syscall_2(9, oldfd, newfd); }

int stat(const char* pathname, struct stat* statbuf) {
  return syscall_2(10, (int64_t)pathname, (int64_t)statbuf);
}

void* sbrk(intptr_t bytes) { return (void*)syscall_1(11, (int64_t)bytes); }

int get_dents(int fd, struct linux_dirent* dirp, size_t count) {
  return (int)syscall_3(12, fd, (int64_t)(dirp), count);
}

char* getcwd(char* buf, size_t size) {
  return (char*)syscall_2(13, (int64_t)buf, size);
}
