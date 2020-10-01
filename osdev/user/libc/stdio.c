#include "stdio.h"

#include <stdlib.h>
#include <string.h>
#include <syscall.h>

#define O_APPEND 1
#define O_CREAT 2
#define O_DIRECTORY 4
#define O_TRUNC 8

FILE _f_stdin = {.fd = 0};
FILE _f_stdout = {.fd = 1};
FILE _f_stderr = {.fd = 2};

FILE *fopen(const char *pathname, const char *mode) {
  FILE *file = (FILE *)malloc(sizeof(FILE));

  int fd;
  if (strcmp(mode, "w") == 0) {
    fd = open(pathname, O_TRUNC);
  } else {
    fd = open(pathname, 0);
  }

  if (fd == -1) {
    return NULL;
  }

  file->fd = fd;
  file->buf_size = 0;
  file->buf_pos = 0;

  (void)mode;

  return file;
}

int fclose(FILE *stream) {
  (void)stream;
  return 0;
}

int fflush(FILE *stream) {
  (void)stream;
  return 0;
}

size_t fread(void *ptr, size_t size, size_t count, FILE *stream) {
  if (size == 0 || count == 0) {
    return 0;
  }

  for (size_t index = 0; index < count; index++) {
    for (size_t i = 0; i < size; i++) {
      int c = fgetc(stream);
      if (c == EOF) {
        return index;
      }
      ((char *)ptr)[index * size + i] = c;
    }
  }

  return count;
}

size_t fwrite(const void *ptr, size_t size, size_t count, FILE *stream) {
  int fd = stream->fd;
  int num_write = write(fd, (char *)ptr, size * count);
  return num_write / size;
}

int fgetc(FILE *stream) {
  if (!stream) {
    return EOF;
  }

  // If the buffer is all read.
  if (stream->buf_size == stream->buf_pos) {
    // Then we need to fill up the buffer.
    int read_cnt = read(stream->fd, stream->buffer, BUF_SIZE);
    stream->buf_size = read_cnt;
    stream->buf_pos = 0;
  }

  // Even after try filling the buffer, if the buffer is still empty.
  if (stream->buf_size == 0) {
    return EOF;
  }

  return stream->buffer[stream->buf_pos++];
}

char *fgets(char *str, int count, FILE *stream) {
  if (stream == NULL) {
    return NULL;
  }

  if (count < 1) {
    return NULL;
  }

  for (int i = 0; i < count - 1; i++) {
    int ch = fgetc(stream);
    if (ch == EOF) {
      // No bytes are read.
      if (i == 0) {
        return NULL;
      }

      str[i] = 0;
      return str;
    } else {
      str[i] = ch;
    }

    if (ch == '\n') {
      str[i + 1] = 0;
      return str;
    }
  }

  str[count - 1] = 0;
  return str;
}

int fputc(int ch, FILE *stream) {
  if (!stream) {
    return EOF;
  }

  // If the buffer is full, then we should flush.
  if (stream->buf_size == BUF_SIZE) {
    write(stream->fd, stream->buffer, stream->buf_size);
    stream->buf_size = 0;
    stream->buf_pos = 0;
  }

  stream->buffer[stream->buf_pos] = ch;
  stream->buf_size++;
  stream->buf_pos++;

  bool should_flush = false;
  if (stream->mode & _IO_UNBUFFERED) {
    should_flush = true;
  } else if ((stream->mode & _IO_LINE_BUFFERED) && ch == '\n') {
    should_flush = true;
  } else if ((stream->mode & _IO_BLOCK_BUFFERED) &&
             stream->buf_size == BUF_SIZE) {
    should_flush = true;
  }

  if (should_flush) {
    write(stream->fd, stream->buffer, stream->buf_size);
    stream->buf_size = 0;
    stream->buf_pos = 0;
  }

  return ch;
}

long ftell(FILE *stream) {
  if (stream == NULL) {
    return EOF;
  }

  return lseek(stream->fd, 0, SEEK_CUR);
}

int fseek(FILE *stream, long offset, int origin) {
  return lseek(stream->fd, offset, origin);
}

int putchar(int ch) {
  printf("%c", ch);
  return ch;
}

int puts(const char *str) { return printf("%s", str); }
