#ifndef LIBC_STDIO_H
#define LIBC_STDIO_H

#include <io.h>
#include <printf.h>
#include <scanf.h>

#define stdin (&_f_stdin)
#define stdout (&_f_stdout)
#define stderr (&_f_stderr)

#define EOF (-1)

#define _IO_UNBUFFERED (1 << 0)
#define _IO_LINE_BUFFERED (1 << 1)
#define _IO_BLOCK_BUFFERED (1 << 2)

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

FILE *fopen(const char *pathname, const char *mode);

// Do nothing.
int fclose(FILE *stream);

// Do nothing.
int fflush(FILE *stream);

size_t fread(void *ptr, size_t size, size_t count, FILE *stream);
size_t fwrite(const void *ptr, size_t size, size_t count, FILE *stream);

int fgetc(FILE *stream);
char *fgets(char *str, int count, FILE *stream);

long ftell(FILE *stream);
int fseek(FILE *stream, long offset, int origin);

int putchar(int ch);
int puts(const char *str);

FILE _f_stdin;
FILE _f_stdout;
FILE _f_stderr;

#endif
