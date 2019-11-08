#ifndef STRING_H
#define STRING_H
#include "types.h"

namespace Kernel {

void* memcpy(void* dest, void* src, size_t count);
void* memset(void* dest, int ch, size_t count);

}  // namespace Kernel
#endif
