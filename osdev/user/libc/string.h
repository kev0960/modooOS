#include <stddef.h>

void* memcpy(void* dest, const void* src, size_t count);
void* memset(void* dest, int ch, size_t count);
void* memmove(void* dest, const void* src, size_t count);
int memcmp(const void* lhs, const void* rhs, size_t count);

int strcmp(const char* lhs, const char* rhs);
int strncmp(const char* lhs, const char* rhs, size_t count);
size_t strlen(const char* str);
char* strchr(const char* str, int ch);

