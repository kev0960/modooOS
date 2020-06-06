#include "stdint.h"

typedef uint64_t size_t;
typedef uint64_t pid_t;

int open(const char* path_name);

size_t read(int64_t fd, char* buf, size_t count);
size_t write(const char* s);

int spawn(pid_t* pid, const char* s);
pid_t waitpid(pid_t pid, int* status);

int pipe(int pipefd[2]);
int dup2(int oldfd, int newfd);

struct stat {
  size_t inode;
  size_t file_size;
  uint16_t mode;
};

int stat(const char* pathname, struct stat* statbuf);
