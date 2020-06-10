#include "malloc.h"

#include "syscall.h"

static size_t heap_size = 0;

// Pre-defined heap start address.
static void* heap_start = (void*)0x10000000;

size_t RoundUpToMultipleOfFourKb(size_t bytes) {
}

void* malloc(unsigned long int bytes) { 
  if (heap_size == 0) {
  }
  return 0; 
}
