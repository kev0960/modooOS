#ifndef LIBC_IO_H
#define LIBC_IO_H

#define BUF_SIZE (4096)

typedef struct _File_IO {
  int fd;  // Descriptor number of the file.

  unsigned char buffer[BUF_SIZE];

  // Size of the current buffer.
  int buf_size;

  // Current (reading/writing) location in buffer.
  int buf_pos;

  int mode;
} FILE;

#endif
