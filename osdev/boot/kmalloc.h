#ifndef KMALLOC_H
#define KMALLOC_H

#include "kernel_paging.h"
#include "types.h"

#define KERNEL_HEAP_MEMORY_START_OFFSET 0x1000000  // 16 MB

// Since each "chunk" has 4 byte prefix and 4 byte suffix, to align every
// returned memory at 8 byte bounday, we have to start with 4 byte margin in the
// beginning.
#define MEMORY_ALGIN_OFFSET 4  // 4 bytes

#define NUM_BUCKETS 16

namespace Kernel {
class KernelMemoryManager {
 public:
  KernelMemoryManager()
      : heap_start_(reinterpret_cast<void*>(KERNEL_VIRTUAL_START +
                                            KERNEL_HEAP_MEMORY_START_OFFSET +
                                            MEMORY_ALGIN_OFFSET)),
        heap_memory_limit_(ONE_GB - KERNEL_HEAP_MEMORY_START_OFFSET -
                           MEMORY_ALGIN_OFFSET),
        current_heap_size_(0) {
    for (int i = 0; i < NUM_BUCKETS; i++) {
      free_list_[i] = nullptr;
    }
  }

  void* GetMemoryFromBucket(int bucket_index, uint32_t bytes);

 private:
  void* SplitMemory(void* addr, uint32_t split_size);

  // Total 8 + memory_size will be allocated.
  void* CreateNewChunkAt(void* addr, uint32_t memory_size);

  void* GetAddressByOffset(int offset_size);

  // Virtual memory where heap start. Because of the identity mapping, the
  // physical address of kernel memory will simply be (current memory address -
  // KERNEL_VIRTUAL_START).
  void* heap_start_;

  // Size of the maximum heap. Since we are using 1 GB paging for the kernel and
  // the kernel VM is 1 GB of identity mapping, the usable heap size will simply
  // be 1 GB - offset.
  const int heap_memory_limit_;

  // Current end of heap.
  int current_heap_size_;

  // Buckets for 2^3, 2^4, ..., 2^18 bytes, total of 16 buckets.
  void* free_list_[NUM_BUCKETS];
};

extern KernelMemoryManager kernel_memory_manager;

void* KernelMalloc(uint32_t bytes);
void KernelFree(void* ptr);

}  // namespace Kernel

#endif