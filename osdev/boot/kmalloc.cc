#include "kmalloc.h"

namespace Kernel {
namespace {

const int kKmallocMagic = 0x7DDDDDDD;
const int kKmallocMagicOccupied = 0xFDDDDDDD;

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
bool IsOccupied(uint8_t* byte) { return (*byte) & 0b10000000; }

// Check the magic value.
bool CheckMagic(uint8_t* suffix) {
  uint32_t* s = reinterpret_cast<uint32_t*>(suffix);
  return (*s) == kKmallocMagic || (*s) == kKmallocMagicOccupied;
}

void SetChunkSize(uint8_t* addr, uint32_t size) {
  // Write the size of the chunk. Note that maximum size of the chunk will be
  // 2^30. That means, the left most bit of size will be 0.
  (*reinterpret_cast<uint32_t*>(addr)) |= size;
}
void SetMagic(uint8_t* addr) {
  (*reinterpret_cast<uint32_t*>(addr)) |= kKmallocMagic;
}

uint32_t GetChunkSize(uint8_t* addr) {
  uint32_t prefix_block = *reinterpret_cast<uint32_t*>(addr);

  // Drop the left most "free" bit.
  return (prefix_block << 1) >> 1;
}

void SetPrevOffset(uint8_t* chunk_addr, uint32_t prev_chunk_offset) {
  uint32_t* addr = reinterpret_cast<uint32_t*>(chunk_addr);
  addr[1] = prev_chunk_offset;
}

void SetNextOffset(uint8_t* chunk_addr, uint32_t next_chunk_offset) {
  uint32_t* addr = reinterpret_cast<uint32_t*>(chunk_addr);
  addr[2] = next_chunk_offset;
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

uint8_t* GetEndOfChunkFromStart(uint8_t* addr, uint32_t memory_size) {
  return addr + memory_size + 8;
}

uint8_t* GetStartOfChunkFromSuffix(uint8_t* suffix) {
  auto mem_size = GetChunkSize(suffix);
  return suffix - mem_size - 4;
}

KernelMemoryManager::PrevAndNext GetPrevAndNextFromFreeChunk(uint8_t* addr) {
  uint32_t* data = reinterpret_cast<uint32_t*>(addr + 4);
  return {data[0], data[1]};
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
  return CreateNewUsedChunkAt(GetAddressByOffset(current_heap_size_),
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
  PrevAndNext prev_and_next = GetPrevAndNextFromFreeChunk(addr);
  uint32_t memory_size = GetChunkSize(addr);

  // If the remaining size is smaller than the smallest possible chunk, then we
  // simply use entire available space.
  if (memory_size - split_size < 16) {
    split_size = memory_size;
  }

  WeaveTwoChunksTogether(prev_and_next, bucket_index);

  // Now assign "split_size" to the new chunk.
  CreateNewUsedChunkAt(addr, split_size);

  memory_size -= split_size;  // Remaining available memory size.
  if (memory_size == 0) {
    return addr;
  }

  // Create new free chunk
  uint8_t* free_chunk_start = GetEndOfChunkFromStart(addr, split_size);
  CreateNewFreeChunkAt(free_chunk_start, memory_size, bucket_index);

  return addr;
}

void KernelMemoryManager::CreateNewFreeChunkAt(uint8_t* addr,
                                               uint32_t memory_size,
                                               int bucket_index) {
  SetFree(addr);
  SetChunkSize(addr, memory_size);

  auto my_offset = GetOffsetFromHeapStart(addr);
  // Add chunk to the free list.
  auto next = free_list_[bucket_index];
  free_list_[bucket_index] = my_offset;

  if (next != 0) {
    uint8_t* chunk = GetAddressByOffset(next);
    SetPrevOffset(chunk, my_offset);
  }

  SetPrevOffset(addr, 0);
  SetNextOffset(addr, next);

  uint8_t* suffix = GetSuffixBlockAddressFromChunkStart(addr, memory_size);
  SetFree(suffix);
  SetChunkSize(suffix, memory_size);
}

uint8_t* KernelMemoryManager::CreateNewUsedChunkAt(uint8_t* addr,
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
  SetChunkSize(addr, memory_size);

  uint8_t* suffix_addr = GetSuffixBlockAddressFromChunkStart(addr, memory_size);
  SetOccupied(suffix_addr);
  SetMagic(suffix_addr);

  return GetMemoryBlockAddressFromChunkStart(addr);
}

// Coalsce Two free chunks.
void KernelMemoryManager::CoalsceTwoChunks(uint8_t* left, uint8_t* right) {
  auto left_size = GetChunkSize(left);
  auto right_size = GetChunkSize(right);

  // Detach left and right chunk from the free list.
  WeaveTwoChunksTogether(GetPrevAndNextFromFreeChunk(left),
                         GetBucketIndex(left_size));
  WeaveTwoChunksTogether(GetPrevAndNextFromFreeChunk(right),
                         GetBucketIndex(right_size));

  int total_size = left_size + right_size + 8;
  int bucket_index = GetBucketIndex(total_size);
  CreateNewFreeChunkAt(left, total_size, bucket_index);
}

void KernelMemoryManager::FreeOccupiedChunk(uint8_t* addr) {
  // Mark current chunk as free.
  auto chunk_size = GetChunkSize(addr);
  CreateNewFreeChunkAt(addr, chunk_size, GetBucketIndex(chunk_size));

  // Check whether the chunk on the left side is free.
  uint8_t* suffix_of_left_chunk = reinterpret_cast<uint8_t*>(addr) - 4;
  if (suffix_of_left_chunk >= heap_start_ &&
      !IsOccupied(suffix_of_left_chunk)) {
    uint8_t* left = GetStartOfChunkFromSuffix(suffix_of_left_chunk);
    CoalsceTwoChunks(left, addr);
    addr = left;
  }

  uint8_t* prefix_of_right_chunk =
      GetEndOfChunkFromStart(addr, GetChunkSize(addr));
  // Check whether the chunk on right side is free.
  if (GetOffsetFromHeapStart(prefix_of_right_chunk) < current_heap_size_ &&
      !IsOccupied(prefix_of_right_chunk)) {
    CoalsceTwoChunks(addr, prefix_of_right_chunk);
  }
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

void KernelMemoryManager::WeaveTwoChunksTogether(
    const PrevAndNext& prev_and_next, int bucket_index) {
  // This means the prev chunk waas just the free_list.
  if (prev_and_next.prev_offset == 0) {
    free_list_[bucket_index] = prev_and_next.next_offset;
  } else {
    uint8_t* chunk = GetAddressByOffset(prev_and_next.prev_offset);
    SetNextOffset(chunk, prev_and_next.next_offset);
  }

  if (prev_and_next.next_offset != 0) {
    uint8_t* chunk = GetAddressByOffset(prev_and_next.next_offset);
    SetPrevOffset(chunk, prev_and_next.prev_offset);
  }
}

void* kmalloc(size_t bytes) {
  if (bytes == 0) {
    return nullptr;
  }

  int bucket_index = GetBucketIndex(bytes);
  return reinterpret_cast<void*>(
      kernel_memory_manager.GetMemoryFromBucket(bucket_index, bytes));
}

void kfree(void* ptr) {
  kernel_memory_manager.FreeOccupiedChunk(reinterpret_cast<uint8_t*>(ptr));
}

}  // namespace Kernel
