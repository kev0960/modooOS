#ifndef PAGING_H
#define PAGING_H

#include "../std/array.h"
#include "../std/types.h"
#include "../std/vector.h"

namespace Kernel {

// Physical frame allocator.
// We use buddy block based allocation.
class FrameAllocator {};

struct FrameDescriptor {
  void* page;
  FrameDescriptor* prev;
  FrameDescriptor* next;

  FrameDescriptor(void* page) : page(page), prev(nullptr), next(nullptr) {}
};

class BuddyBlockAllocator {
 public:
  BuddyBlockAllocator(uint8_t* const start_phys_addr);

  // This gives 2^order frames (total 2^order * 4kb).
  void* GetFrame(size_t order);

  void FreeFrame(void* addr);

 private:
  static constexpr int kBuddyBlockAllocatorOrder = 13;
  static constexpr size_t kFrameSize = 4 * 1024;  // 4KB
  static constexpr size_t kFrameSizeOrder = 12;  // 4KB

  // Split blocks to allocate "order" size pages from chunk in free_list_index.
  void Split(size_t free_list_index, size_t order);

  template <typename T>
  size_t GetOffset(T* addr) {
    return reinterpret_cast<size_t>(addr) -
           reinterpret_cast<size_t>(start_phys_addr_);
  }

  size_t FlipBlockSplitted(size_t offset, size_t order);

  void AddToFreeList(size_t free_list_index, size_t offset);
  FrameDescriptor* RemoveFirstFromFreeList(size_t free_list_index);

  // The physical address that this allocator starts.
  uint8_t* const start_phys_addr_;

  // Each bit indicates whether certain block should be merged or not.
  std::vector<int> free_leaf_info_;

  std::vector<int> is_block_splitted_;

  std::array<FrameDescriptor*, kBuddyBlockAllocatorOrder + 1> free_lists_;
};

class UserPageAllocator {};
}  // namespace Kernel

#endif
