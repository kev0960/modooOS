#include "stdio.h"

#include <stdlib.h>
#include <syscall.h>

static bool CheckReadInBuffer(FILE *f, off_t offset) {
  off_t offset_in_buffer = offset - f->read_buf_start_offset;
  return (0 <= offset_in_buffer && offset_in_buffer < f->read_buffer_size);
}

static bool CheckInWriteBuffer(FILE *f, off_t offset) {
  off_t offset_in_buffer = offset - f->write_buf_start_offset;
  return (0 <= offset_in_buffer && offset_in_buffer < f->write_buffer_size);
}

FILE *fopen(const char *pathname, const char *mode) {
  FILE *file = (FILE *)malloc(sizeof(FILE));
  int fd = open(pathname);

  if (fd == -1) {
    return NULL;
  }

  file->fd = fd;
  file->read_buf_start_offset = 0;
  file->read_buffer_size = 0;
  file->current_read = 0;

  file->write_buf_start_offset = 0;
  file->current_write = 0;
  file->write_buffer_size = 0;

  (void)mode;

  return file;
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

  if (CheckReadInBuffer(stream, stream->current_read)) {
    // Simple read current buffer.
    int index = stream->current_read - stream->read_buf_start_offset;
    stream->current_read++;
    return stream->read_buffer[index];
  }

  // If not, then let's fill the buffer!
  int num_read =
      pread(stream->fd, stream->read_buffer, BUF_SIZE, stream->current_read);
  if (num_read == 0) {
    return EOF;
  }

  stream->read_buf_start_offset = stream->current_read;
  stream->read_buffer_size = num_read;
  stream->current_read++;

  return stream->read_buffer[0];
}

int fputc(int ch, FILE *stream) {
  if (!stream) {
    return EOF;
  }

  off_t written_index;

  if (CheckInWriteBuffer(stream, stream->current_write)) {
    off_t index = stream->current_write - stream->write_buf_start_offset;
    stream->current_write++;

    stream->write_buffer[index] = ch;
    goto check_overlap;
  }

  // If write buffer size is not BUF_SIZE, then the write buffer touching EOF.
  // In that case, we don't have to really call pread.
  if (stream->write_buffer_size < BUF_SIZE) {
    stream->write_buffer_size++;

    off_t index = stream->current_write - stream->write_buf_start_offset;
    stream->current_write++;

    stream->write_buffer[index] = ch;
    goto check_overlap;
  }

  int num_read =
      pread(stream->fd, stream->write_buffer, BUF_SIZE, stream->current_write);
  stream->write_buf_start_offset = stream->current_write;
  stream->write_buffer_size = (num_read > 0 ? num_read : 1);

  stream->current_write++;
  stream->write_buffer[0] = ch;

  // Check if there is an overlap between read buffer and write buffer.
  // If it is, then we have to write at the read buffer too.
check_overlap:
  written_index = stream->current_write - 1;
  if (CheckReadInBuffer(stream, written_index)) {
    int index = written_index - stream->read_buf_start_offset;
    stream->read_buffer[index] = ch;
  }

  return ch;
}
