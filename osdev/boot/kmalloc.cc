#include "kmalloc.h"

namespace Kernel {
namespace {
int GetNearestPowerOfTwoLog(uint32_t bytes) {
  // For example,
  // 0000 0000 | 0000 0000 | 0000 0001 | 0010 0000
  // num_leading_zeros : 23
  // num_trailing_zeros : 5
  // --> returns 32 - 23 + 1 = 10 (2^10)
  //
  // 0000 0000 | 0000 0000 | 0000 0001 | 0000 0000
  // num_leading_zeros : 23
  // num_trailing_zeros : 8 (+ 1) = 9
  // --> returns 9.
  int num_leading_zeros = __builtin_clz(bytes);
  int num_trailing_zeros = __builtin_ffs(bytes);

  // bytes is the power of 2.
  if (num_leading_zeros + num_trailing_zeros == 32) {
    return num_trailing_zeros;
  } else {
    return 32 - num_leading_zeros + 1;
  }
}

int GetNearestPowerOfTwo(uint32_t bytes) {
  return 1 << GetNearestPowerOfTwoLog(bytes);
}

int GetBucketIndex(uint32_t bytes) {
  int power = GetNearestPowerOfTwoLog(bytes);
  if (power <= 3) {
    return 0;
  } else if (3 < power && power < 3 + NUM_BUCKETS) {
    return power - 3;
  }
  return NUM_BUCKETS - 1;
}

void SetOccupied(uint8_t* byte) { (*byte) |= 0b10000000; }
void SetFree(uint8_t* byte) { (*byte) &= 0b01111111; }
void SetChunkSize(uint32_t* addr, uint32_t size) {
  // Write the size of the chunk. Note that maximum size of the chunk will be
  // 2^30. That means, the left most bit of size will be 0.
  (*addr) |= size;
}
uint32_t GetChunkSize(uint8_t* addr) {
  uint32_t prefix_block = *reinterpret_cast<uint32_t*>(addr);

  // Drop the left most "free" bit.
  return (prefix_block << 1) >> 1;
}

void SetNextChunkOffset(uint32_t* addr, uint32_t next_chunk_offset) {
  // left-most bit is obviously 0 considering the total size of heap memory.
  // Hence, this will not erase the free bit.
  (*addr) |= next_chunk_offset;
}

uint32_t GetNextChunkOffset(uint8_t* addr) {
  uint32_t suffix_block = *reinterpret_cast<uint32_t*>(addr);
  return (suffix_block << 1) >> 1;
}

uint32_t GetChunkSize(uint32_t* addr) {
  uint32_t data = *addr;

  // Drop the left most bit (which is Free bit).
  return (data << 1) >> 1;
}

// 4 byte prefix.
uint8_t* GetMemoryBlockAddressFromChunkStart(uint8_t* addr) {
  return (addr + 4);
}

uint8_t* GetSuffixBlockAddressFromChunkStart(uint8_t* addr,
                                             uint32_t memory_size) {
  return addr + memory_size + 4;
}

}  // namespace

KernelMemoryManager kernel_memory_manager;

uint8_t* KernelMemoryManager::GetMemoryFromBucket(int bucket_index,
                                                  uint32_t bytes) {
  // Find if there is available free chunk of memory.
  for (int i = bucket_index; i < NUM_BUCKETS; i++) {
    auto free_chunk_offset = IterateFreeList(free_list_[i], bytes);
    if (free_chunk_offset) {
      return SplitMemory(GetAddressByOffset(free_chunk_offset), bytes, i);
    }
  }

  // If no memory free memory is available.
  size_t allocate_size = GetNearestPowerOfTwo(bytes);
  return CreateNewChunkAt(GetAddressByOffset(current_heap_size_),
                          allocate_size);
}

uint32_t KernelMemoryManager::IterateFreeList(uint32_t free_list,
                                              uint32_t memory_size) {
  uint32_t current_offset = free_list;
  while (current_offset) {
    uint8_t* current_chunk = GetAddressByOffset(current_offset);

    // Get the size of available memory.
    uint32_t chunk_mem_size = GetChunkSize(current_chunk);
    if (chunk_mem_size >= memory_size) {
      return current_offset;
    }

    // Otherwise, find the next available free chunk.
    uint8_t* suffix_chunk =
        GetSuffixBlockAddressFromChunkStart(current_chunk, chunk_mem_size);
    current_offset = GetNextChunkOffset(suffix_chunk);
  }

  return 0;
}

uint8_t* KernelMemoryManager::SplitMemory(uint8_t* addr, uint32_t split_size,
                                          int bucket_index) {
  

}

uint8_t* KernelMemoryManager::CreateNewChunkAt(uint8_t* addr,
                                               uint32_t memory_size) {
  size_t allocated_size = memory_size + 8;

  // Increase the heap size by allocated size. If not possible, return nullptr.
  if (current_heap_size_ + allocated_size >= heap_memory_limit_) {
    return nullptr;
  }
  current_heap_size_ += allocated_size;

  // Mark that the chunk is occupied.
  SetOccupied(addr);

  // Mark the size of the chunk.
  SetChunkSize(reinterpret_cast<uint32_t*>(addr), memory_size);

  // Add the chunk at the bucket.
  uint32_t next_chunk_in_bucket = 0;
  if (!free_list_[bucket_index]) {
    free_list_[bucket_index] = GetOffsetFromHeapStart(addr);
  } else {
    next_chunk_in_bucket = free_list_[bucket_index];
    free_list_[bucket_index] = GetOffsetFromHeapStart(addr);
  }

  uint8_t* suffix_addr = GetSuffixBlockAddressFromChunkStart(addr, memory_size);
  SetNextChunkOffset(reinterpret_cast<uint32_t*>(suffix_addr),
                     next_chunk_in_bucket);
  SetOccupied(suffix_addr);

  return GetMemoryBlockAddressFromChunkStart(addr);
}

uint8_t* KernelMemoryManager::GetAddressByOffset(uint32_t offset_size) const {
  return heap_start_ + offset_size;
}

uint32_t KernelMemoryManager::GetOffsetFromHeapStart(uint8_t* addr) const {
  return reinterpret_cast<uint64_t>(addr) -
         reinterpret_cast<uint64_t>(heap_start_);
}

uint8_t* KernelMemoryManager::GetActualMemoryAddressByOffset(
    uint32_t offset_size) const {
  return heap_start_ + offset_size + /* prefix size = */ 4;
}

void* kmalloc(size_t bytes) {
  if (bytes == 0) {
    return nullptr;
  }

  int bucket_index = GetBucketIndex(bytes);
  return reinterpret_cast<void*>(
      kernel_memory_manager.GetMemoryFromBucket(bucket_index, bytes));
}

void kfree(void* ptr) {}

}  // namespace Kernel
