#ifndef LIBC_UNISTD_H
#define LIBC_UNISTD_H

#include <stdint.h>

#ifndef ssize_t_defined
typedef int64_t ssize_t;
#define ssize_t_defined
#endif

#define SEEK_SET 0 /* set file offset to offset */
#define SEEK_CUR 1 /* set file offset to current plus offset */
#define SEEK_END 2 /* set file offset to EOF plus offset */

#define STDIN_FILENO 0  /* Standard input.  */
#define STDOUT_FILENO 1 /* Standard output.  */
#define STDERR_FILENO 2 /* Standard error output.  */

#endif
