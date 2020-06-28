#include "stdint.h"

typedef uint64_t size_t;
typedef uint64_t pid_t;

int open(const char* path_name);

size_t read(int64_t fd, char* buf, size_t count);
size_t write(int fd, const char* s, size_t count);

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
void* sbrk(intptr_t bytes);

struct linux_dirent {
  uint32_t d_ino;    /* Inode number */
  uint32_t d_off;    /* Offset to next linux_dirent */
  uint16_t d_reclen; /* Length of this linux_dirent */
  char d_name[];     /* Filename (null-terminated) */
                     /* length is actually (d_reclen - 2 -
                        offsetof(struct linux_dirent, d_name) */
};

int get_dents(int fd, struct linux_dirent* dirp, size_t count);

char* getcwd(char* buf, size_t size);

enum ConsoleCommand { SET_NO_BUFFER, SET_LINE_BUFFER };

int console(enum ConsoleCommand command);
