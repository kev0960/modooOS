#include "memory.h"

// Overriding global new and delete.
// NOTE THAT WE DO NOT USE new[] or delete[] IN KERNEL CONTEXT.
void* operator new(Kernel::size_t count) { return Kernel::kmalloc(count); }
void* operator new(Kernel::size_t, void* p) { return p; }

void operator delete(void* p) noexcept { Kernel::kfree(p); }
void operator delete(void* p, Kernel::size_t) noexcept { Kernel::kfree(p); }
