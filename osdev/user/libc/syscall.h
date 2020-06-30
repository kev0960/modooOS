#include "stdint.h"

typedef uint64_t size_t;
typedef uint64_t pid_t;
typedef int64_t off_t;

int open(const char* path_name);

size_t read(int64_t fd, char* buf, size_t count);
size_t pread(int64_t fd, char* buf, size_t count, off_t offset);

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

enum ConsoleCommand {
  SET_NO_BUFFER = 1,
  SET_LINE_BUFFER = 2,
  SET_ASCII = 4,
  SET_EVERYTHING = 8,
  SET_BLOCKING_IO = 16,
  SET_NON_BLOCKING_IO = 32,
  SET_RECORD_UP = 64,
};

int console(enum ConsoleCommand command);

enum ScreenCommands { GET_SCREEN_INFO, COPY_FRAME_BUFFER };

struct ScreenInfo {
  int width;
  int height;
  int pixel_size;
};

struct FrameBufferInfo {
  // Location to draw at.
  int screen_row;
  int screen_col;

  // Size of the buffer.
  int buffer_width;
  int buffer_height;
};

int screen(enum ScreenCommands command, void* arg1, void* arg2);

int usleep(size_t microseconds);
size_t mstick();
