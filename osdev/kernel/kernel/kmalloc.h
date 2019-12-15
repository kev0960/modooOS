#ifndef KMALLOC_H
#define KMALLOC_H

#include "../boot/kernel_paging.h"
#include "../std/types.h"

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
      : heap_start_(reinterpret_cast<uint8_t*>(KERNEL_VIRTUAL_START +
                                               KERNEL_HEAP_MEMORY_START_OFFSET +
                                               MEMORY_ALGIN_OFFSET)),
        heap_memory_limit_(ONE_GB - KERNEL_HEAP_MEMORY_START_OFFSET -
                           MEMORY_ALGIN_OFFSET),
        current_heap_size_(8 /* Added initial offset*/) {
    for (int i = 0; i < NUM_BUCKETS; i++) {
      free_list_[i] = 0;
    }
  }

  uint8_t* GetMemoryFromBucket(int bucket_index, uint32_t bytes);
  void FreeOccupiedChunk(uint8_t* addr);
  bool CheckMemoryDeleteSize(uint8_t* addr, uint32_t bytes) const;

  void* AlignedAlloc(size_t alignment, size_t bytes);

  struct PrevAndNext {
    uint32_t prev_offset;
    uint32_t next_offset;
  };

  void ShowDebugInfo() const;

  // Resets entire heap allocation. All the previously allocated memory will be
  // unusable. ONLY USE THIS FOR TESTING PURPOSES!
  void Reset();
  bool SanityCheck();
  void DumpMemory();

 private:
  uint8_t* SplitMemory(uint8_t* addr, uint32_t split_size, int bucket_index);

  // Total 8 + memory_size will be allocated.
  uint8_t* CreateNewUsedChunkAt(uint8_t* addr, uint32_t memory_size);

  void CreateNewFreeChunkAt(uint8_t* addr, uint32_t memory_size,
                            int bucket_index);

  uint8_t* GetAddressByOffset(uint32_t offset_size) const;

  // Get the actual memory address (not including the prefix info).
  uint8_t* GetActualMemoryAddressByOffset(uint32_t offset_size) const;
  uint32_t GetOffsetFromHeapStart(uint8_t* addr) const;

  uint32_t IterateFreeList(uint32_t free_list, uint32_t memory_size);

  // Weave two chunks in free list together.
  void WeaveTwoChunksTogether(const PrevAndNext& prev_and_next,
                              int bucket_index);

  void CoalsceTwoChunks(uint8_t* left, uint8_t* right);

  // Completely remove free chunk from heap. The chunk MUST be at the end of the
  // heap boundary.
  void RemoveFreeChunkFromHeap(uint8_t* addr);

  // Virtual memory where heap start. Because of the identity mapping, the
  // physical address of kernel memory will simply be (current memory address -
  // KERNEL_VIRTUAL_START).
  uint8_t* const heap_start_;

  // Size of the maximum heap. Since we are using 1 GB paging for the kernel and
  // the kernel VM is 1 GB of identity mapping, the usable heap size will simply
  // be 1 GB - offset.
  const uint32_t heap_memory_limit_;

  // Current end of heap.
  uint32_t current_heap_size_;

  // Buckets for 2^3, 2^4, ..., 2^18 bytes, total of 16 buckets.
  // Each bucket contains the offset to the available memory chunk.
  uint32_t free_list_[NUM_BUCKETS];
};

extern KernelMemoryManager kernel_memory_manager;

void* kmalloc(size_t bytes);
void* kcalloc(size_t bytes);
void kfree(void* ptr);

void* kaligned_alloc(size_t alignment, size_t bytes);

}  // namespace Kernel

#endif
