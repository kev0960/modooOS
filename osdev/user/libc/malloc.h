#ifndef LIBC_MALLOC_H
#define LIBC_MALLOC_H

#include <stdbool.h>

void* malloc(unsigned long int bytes);
void* realloc(void* ptr, unsigned long int bytes);
void free(void* mem);

void __print_free_list();
bool __malloc_sanity_check();
void __malloc_show_status();

#endif
