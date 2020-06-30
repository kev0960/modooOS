#ifndef LIBC_STDIO_H
#define LIBC_STDIO_H

#include <printf.h>

#define stdin (0)
#define stdout (1)
#define stderr (2)

#define EOF (-1)
#define BUF_SIZE (128)

typedef struct FILE {
  int fd;  // Descriptor number of the file.

  // File offset of the head of the buffer.
  size_t read_buf_start_offset;
  char read_buffer[BUF_SIZE];
  int read_buffer_size;

  // This is offset relative to the head of the FILE (NOT Buffer)
  // To get the offset in the buffer, then we need to do (current_read -
  // read_buf_start_offset) and check it is between 0 and read_buffer_size.
  int current_read;

  size_t write_buf_start_offset;
  char write_buffer[BUF_SIZE];
  int write_buffer_size;

  // This is offset relative to the head of the FILE (NOT Buffer)
  int current_write;

  int mode;
} FILE;

FILE *fopen(const char *pathname, const char *mode);
size_t fread(void *ptr, size_t size, size_t count, FILE *stream);
size_t fwrite(const void *ptr, size_t size, size_t count, FILE *stream);

int fgetc(FILE *stream);
char *fgets(char *str, int count, FILE *stream);

#endif
