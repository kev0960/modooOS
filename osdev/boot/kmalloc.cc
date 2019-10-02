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

void SetOccupied(char* byte) { (*byte) |= 0b10000000; }
void SetFree(char* byte) { (*byte) &= 0b01111111; }

// 4 byte prefix.
void* GetMemoryBlockAddressFromChunkStart(void* addr) {
  return reinterpret_cast<void*>(reinterpret_cast<char*>(addr) + 4);
}

}  // namespace

KernelMemoryManager kernel_memory_manager;

void* KernelMemoryManager::GetMemoryFromBucket(int bucket_index,
                                               uint32_t bytes) {
  // Find if there is available free chunk of memory.
  for (int i = bucket_index; i < NUM_BUCKETS; i++) {
    if (free_list_[i] != nullptr) {
      return SplitMemory(free_list_[i], bytes);
    }
  }

  // If no memory free memory is available.
  size_t allocate_size = GetNearestPowerOfTwo(bytes);
  return CreateNewChunkAt(GetAddressByOffset(current_heap_size_),
                          allocate_size);
}

void* KernelMemoryManager::CreateNewChunkAt(void* addr, uint32_t memory_size) {
  size_t allocated_size = memory_size + 8;

  // Increase the heap size by allocated size. If not possible, return nullptr.
  if (current_heap_size_ + allocated_size >= heap_memory_limit_) {
    return nullptr;
  }

  // Mark that the chunk is occupied.
  SetOccupied(reinterpret_cast<char*>(addr));

  return GetMemoryBlockAddressFromChunkStart(addr);
}

void* KernelMemoryManager::GetAddressByOffset(int offset_size) {
  return reinterpret_cast<void*>(reinterpret_cast<char*>(heap_start_) +
                                 offset_size);
}

void* KernelMalloc(size_t bytes) {
  if (bytes == 0) {
    return nullptr;
  }

  int bucket_index = GetBucketIndex(bytes);
  return kernel_memory_manager.GetMemoryFromBucket(bucket_index, bytes);
}

void KernelFree(void* ptr) {}

}  // namespace Kernel
