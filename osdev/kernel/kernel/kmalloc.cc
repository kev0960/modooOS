#include "kmalloc.h"

#include "../std/printf.h"
#include "../std/string.h"
#include "apic.h"
#include "kernel_math.h"
#include "kernel_util.h"

namespace Kernel {
namespace {

int RoundDownNearestPowerOfTwoLog(uint32_t bytes) {
  int num_leading_zeros = __builtin_clz(bytes);

  // bytes is the power of 2.
  return 31 - num_leading_zeros;
}

int RoundUpNearestPowerOfTwo(uint32_t bytes) {
  return 1 << RoundUpNearestPowerOfTwoLog(bytes);
}

bool IsPowerOfTwo(uint32_t x) { return (x & (x - 1)) == 0; }

int GetBucketIndexOfFreeChunk(uint32_t bytes) {
  int power = RoundDownNearestPowerOfTwoLog(bytes);
  if (power <= 2) {
    kprintf("kmalloc : Cannot create bucket with too small memory %d \n",
            bytes);
    return -1;
  } else if (3 <= power && power < 3 + NUM_BUCKETS) {
    return power - 3;
  }
  return NUM_BUCKETS - 1;
}

int GetBucketIndex(uint32_t bytes) {
  int power = RoundUpNearestPowerOfTwoLog(bytes);
  if (power < 3) {
    return 0;
  } else if (3 < power && power < 3 + NUM_BUCKETS) {
    return power - 3;
  }
  return NUM_BUCKETS - 1;
}

// Last bit of first 8 bit is the free bit. This is due to the little endian
// representation.
// Low addr ------------------------> High addr
// [wwww wwwF] [xxxx xxxx] [yyyy yyyy] [zzzz zzzz]
//
// (Note that zz.... www are where size of the chunk is stored.)
//
// When we convert above into uint32_t, it becomes
// [zzzz zzzz] [yyyy yyyy] [xxxx xxxx] [wwww wwwF]
void SetOccupied(uint8_t* byte) { (*byte) |= 0b00000001; }
void SetFree(uint8_t* byte) { (*byte) &= 0b11111110; }
bool IsOccupied(uint8_t* byte) { return (*byte) & 0b000000001; }

void SetChunkSize(uint8_t* addr, uint32_t size) {
  // Write the size of the chunk. Note that maximum size of the chunk will be
  // 2^30. We have enough space to write down the size.

  // First clear the "size" area.
  (*reinterpret_cast<uint32_t*>(addr)) &= 1;

  // Now set the actual size.
  (*reinterpret_cast<uint32_t*>(addr)) |= (size << 1);
}

uint32_t GetChunkSize(uint8_t* addr) {
  uint32_t prefix_block = *reinterpret_cast<uint32_t*>(addr);

  // Drop the left most "free" bit.
  return prefix_block >> 1;
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

// 4 byte prefix.
uint8_t* GetMemoryBlockAddressFromChunkStart(uint8_t* addr) {
  return (addr + 4);
}

uint8_t* GetChunkStartFromMemoryBlockAddress(uint8_t* addr) {
  return (addr - 4);
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
      return GetMemoryBlockAddressFromChunkStart(
          SplitMemory(GetAddressByOffset(free_chunk_offset), bytes, i));
    }
  }

  // If no memory free memory is available.
  size_t allocate_size = RoundUpNearestPowerOfTwo(bytes);
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

  // Poping current free chunk from free list.
  WeaveTwoChunksTogether(prev_and_next, bucket_index);

  // Now assign "split_size" to the new chunk.
  CreateNewUsedChunkAt(addr, split_size);

  if (memory_size == split_size) {
    return addr;
  }

  // Remaining available memory size.
  auto remaining_size = memory_size - split_size - /* info block */ 8;
  uint8_t* free_chunk_start = GetEndOfChunkFromStart(addr, split_size);

  // Create new free chunk
  int bucket_index_of_remaining = GetBucketIndexOfFreeChunk(remaining_size);
  CreateNewFreeChunkAt(free_chunk_start, remaining_size,
                       bucket_index_of_remaining);

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
  uint32_t allocated_size = memory_size + 8;

  // Increase the heap size by allocated size. If not possible, return nullptr.
  if (current_heap_size_ + allocated_size >= heap_memory_limit_) {
    return nullptr;
  }
  if (GetOffsetFromHeapStart(addr) == current_heap_size_) {
    current_heap_size_ += allocated_size;
  }

  // Mark that the chunk is occupied.
  SetOccupied(addr);

  // Mark the size of the chunk.
  SetChunkSize(addr, memory_size);

  uint8_t* suffix_addr = GetSuffixBlockAddressFromChunkStart(addr, memory_size);
  SetOccupied(suffix_addr);
  SetChunkSize(suffix_addr, memory_size);

  return GetMemoryBlockAddressFromChunkStart(addr);
}

// Coalsce Two free chunks.
void KernelMemoryManager::CoalsceTwoChunks(uint8_t* left, uint8_t* right) {
  auto left_size = GetChunkSize(left);
  auto right_size = GetChunkSize(right);

  // Detach left and right chunk from the free list.
  WeaveTwoChunksTogether(GetPrevAndNextFromFreeChunk(left),
                         GetBucketIndexOfFreeChunk(left_size));
  WeaveTwoChunksTogether(GetPrevAndNextFromFreeChunk(right),
                         GetBucketIndexOfFreeChunk(right_size));

  int total_size = left_size + right_size + 8;
  int bucket_index = GetBucketIndexOfFreeChunk(total_size);
  CreateNewFreeChunkAt(left, total_size, bucket_index);
}

void KernelMemoryManager::FreeOccupiedChunk(uint8_t* addr) {
  // Mark current chunk as free.
  auto chunk_size = GetChunkSize(addr);
  CreateNewFreeChunkAt(addr, chunk_size, GetBucketIndexOfFreeChunk(chunk_size));

  // Check whether the chunk on the left side is free.
  uint8_t* suffix_of_left_chunk = reinterpret_cast<uint8_t*>(addr) - 4;
  if (suffix_of_left_chunk >= heap_start_ + 8 /* initial offset */ &&
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

  auto merged_chunk_size = GetChunkSize(addr);
  if (GetOffsetFromHeapStart(addr) + merged_chunk_size + 8 ==
      current_heap_size_) {
    RemoveFreeChunkFromHeap(addr);
  }
}

void KernelMemoryManager::RemoveFreeChunkFromHeap(uint8_t* addr) {
  auto chunk_size = GetChunkSize(addr);

  // Remve current chunk from the free list.
  WeaveTwoChunksTogether(GetPrevAndNextFromFreeChunk(addr),
                         GetBucketIndexOfFreeChunk(chunk_size));

  // Decrease the length of the heap memory.
  current_heap_size_ -= (chunk_size + 8);
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
  // This means the prev chunk was just the free_list.
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

void KernelMemoryManager::ShowDebugInfo() const {
  kprintf("---------- kmalloc Debug Info ---------- \n");
  kprintf("Current Total heap size : %d \n", current_heap_size_ - 8);
  kprintf("---------- Free list offsets  ---------- \n");
  for (int i = 0; i < NUM_BUCKETS; i++) {
    kprintf("%10d|", free_list_[i]);
  }
}

void KernelMemoryManager::Reset() {
  current_heap_size_ = 8;
  for (int i = 0; i < NUM_BUCKETS; i++) {
    free_list_[i] = 0;
  }
}

bool KernelMemoryManager::CheckMemoryDeleteSize(uint8_t* addr,
                                                uint32_t bytes) const {
  auto chunk_size = GetChunkSize(addr - /* Get suffix block */ 4);
  return chunk_size >= bytes;
}

bool KernelMemoryManager::SanityCheck() {
  // First scan the entire heap from the start to the end.
  uint32_t current_offset = 8;
  while (current_offset < current_heap_size_) {
    uint8_t* chunk = GetAddressByOffset(current_offset);
    uint32_t chunk_size = GetChunkSize(chunk);

    if (IsOccupied(chunk)) {
      uint8_t* suffix = GetSuffixBlockAddressFromChunkStart(chunk, chunk_size);
      if (GetChunkSize(suffix) != chunk_size) {
        kprintf("Chunk Corrupted at : %d of %x \n", current_offset, chunk);
        return false;
      }
    }

    current_offset += (8 + chunk_size);
  }
  return true;
}

void KernelMemoryManager::DumpMemory() {
  uint32_t current_offset = 8;
  while (current_offset < current_heap_size_) {
    uint8_t* chunk = GetAddressByOffset(current_offset);
    uint32_t chunk_size = GetChunkSize(chunk);
    kprintf("Offset [%d] Chunk Size : [%d] ", current_offset, chunk_size);

    bool is_occupied = IsOccupied(chunk);
    if (is_occupied) {
      kprintf("O ");
    } else {
      kprintf("F ");
    }

    if (is_occupied) {
      uint8_t* suffix = GetSuffixBlockAddressFromChunkStart(chunk, chunk_size);
      if (GetChunkSize(suffix) != chunk_size) {
        kprintf("CHUNK SUFFIX CORRUPTED! %x", *(uint32_t*)(suffix));
      }
    } else {
      auto prev_and_next = GetPrevAndNextFromFreeChunk(chunk);
      kprintf(" prev : [%d] next : [%d]", prev_and_next.prev_offset,
              prev_and_next.next_offset);
    }

    kprintf("\n");

    current_offset += (8 + chunk_size);
  }
}

void* KernelMemoryManager::AlignedAlloc(size_t alignment, size_t bytes) {
  // First check that bytes is the power of 2.
  if (!IsPowerOfTwo(alignment)) {
    kprintf("The aligned address is not power of 2");
    return nullptr;
  }

  // Allocate bytes + alignment - 1.
  size_t memory_block =
      reinterpret_cast<size_t>(kmalloc(bytes + alignment - 1));
  uint8_t* chunk = GetChunkStartFromMemoryBlockAddress(
      reinterpret_cast<uint8_t*>(memory_block));
  size_t alloc_size = GetChunkSize(chunk);

  // Now find the address that is the multiple of alignment.
  size_t aligned_addr = (memory_block / alignment) * alignment;
  if (aligned_addr < memory_block) {
    aligned_addr += alignment;
  }

  // Now we have free any address before aligned_addr.
  size_t left_free_size = aligned_addr - memory_block;

  // Merge the left free space to pre-exising left chunk.
  if (left_free_size > 0) {
    uint8_t* suffix_of_left_chunk = chunk - 4;
    if (suffix_of_left_chunk >= heap_start_ + 8 /* initial offset */) {
      uint8_t* left = GetStartOfChunkFromSuffix(suffix_of_left_chunk);
      size_t left_memory_size = GetChunkSize(left) + left_free_size;

      // Increase the size of the left space.
      SetChunkSize(left, left_memory_size);

      // Also set new suffix.
      uint8_t* suffix =
          GetSuffixBlockAddressFromChunkStart(left, left_memory_size);
      if (IsOccupied(left)) {
        SetOccupied(suffix);
      } else {
        SetFree(suffix);
      }
      SetChunkSize(suffix, left_memory_size);
    } else {
      if (left_free_size >= 16) {
        // We can create a new free node here. In this case, we should think of
        // the size of the new prefix and suffix.
        int bucket_index_of_remaining =
            GetBucketIndexOfFreeChunk(left_free_size - 8);
        CreateNewFreeChunkAt(chunk, left_free_size - 8,
                             bucket_index_of_remaining);
      } else {
        kprintf("We have unusuable %d bytes of memory :( \n", left_free_size);
      }
    }

    // Now we have to create new prefix for the aligned memory.
    uint8_t* aligned_chunk = GetChunkStartFromMemoryBlockAddress(
        reinterpret_cast<uint8_t*>(aligned_addr));
    SetOccupied(aligned_chunk);
    size_t new_alloc_size = alloc_size - left_free_size;
    SetChunkSize(aligned_chunk, new_alloc_size);

    uint8_t* suffix =
        GetSuffixBlockAddressFromChunkStart(aligned_chunk, new_alloc_size);
    SetOccupied(suffix);
    SetChunkSize(suffix, new_alloc_size);
  }

  return reinterpret_cast<void*>(aligned_addr);
}

void KernelMemoryManager::Lock() {
  if (APICManager::GetAPICManager().IsMulticoreEnabled()) {
    multi_core_lock_.lock();
  } else if (KernelThread::kInitThreadDone) {
    heap_lock_.Down();
  }
}

void KernelMemoryManager::UnLock() {
  if (APICManager::GetAPICManager().IsMulticoreEnabled()) {
    multi_core_lock_.unlock();
  } else if (KernelThread::kInitThreadDone) {
    heap_lock_.Up();
  }
}

void* kmalloc(size_t bytes) {
  if (bytes == 0) {
    return nullptr;
  }

  if (bytes < 8) {
    bytes = 8;
  }

  kernel_memory_manager.Lock();
  int bucket_index = GetBucketIndex(bytes);
  void* mem = reinterpret_cast<void*>(
      kernel_memory_manager.GetMemoryFromBucket(bucket_index, bytes));
  kernel_memory_manager.UnLock();

  return mem;
}

void* kcalloc(size_t bytes) {
  void* addr = kmalloc(bytes);
  if (addr != nullptr) {
    memset(addr, 0, bytes);
  }
  return addr;
}

void kfree(void* ptr) {
  // Move addr to point start of the chunk.
  uint8_t* addr = reinterpret_cast<uint8_t*>(ptr);
  addr -= 4;

  kernel_memory_manager.Lock();
  kernel_memory_manager.FreeOccupiedChunk(addr);
  kernel_memory_manager.UnLock();
}

void* kaligned_alloc(size_t alignment, size_t bytes) {
  // No need to guard here (it internally calls kmalloc!)
  return kernel_memory_manager.AlignedAlloc(alignment, bytes);
}

}  // namespace Kernel
