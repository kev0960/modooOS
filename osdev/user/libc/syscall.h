#include "stdint.h"

typedef uint64_t size_t;
typedef uint64_t pid_t;

void write(const char* s);
int spawn(pid_t* pid, const char* s);
