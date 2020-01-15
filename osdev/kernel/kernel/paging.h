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
  BuddyBlockAllocator(uint8_t* const start_phys_addr,
                      int buddy_block_allocator_order = 13,
                      size_t frame_size = 4 * 1024 /* 4 KB */
  );

  // This gives 2^order frames (total 2^order * 4kb).
  void* GetFrame(size_t order);

  void FreeFrame(void* addr);

  // USE FOR DEBUGGING
  void PrintSplitStatus();

 private:
  // Split blocks to allocate "order" size pages from chunk in free_list_index.
  // That means it will keep split the block until it reaches order.
  // Note: size of the chunk = 2^{order}
  void Split(size_t free_list_index, size_t order);

  // Merge splitted chunk.
  void MergeChunk(size_t offset, size_t order);

  template <typename T>
  size_t GetOffset(T* addr) {
    return reinterpret_cast<size_t>(addr) -
           reinterpret_cast<size_t>(start_phys_addr_);
  }

  void* GetAddrFromOffset(size_t offset) {
    return reinterpret_cast<void*>(offset -
                                   reinterpret_cast<size_t>(start_phys_addr_));
  }

  size_t FlipBlockSplitted(size_t offset, size_t order);
  bool IsBlockSplitted(size_t offset, size_t order) const;

  void AddToFreeList(size_t free_list_index, size_t offset);
  FrameDescriptor* RemoveFirstFromFreeList(size_t free_list_index);
  void RemovePageFromFreeList(size_t free_list_index, FrameDescriptor* desc);
  FrameDescriptor* FindPageFromFreeList(size_t free_list_index,
                                        void* page_addr);

  // The physical address that this allocator starts.
  uint8_t* const start_phys_addr_;

  const int kBuddyBlockAllocatorOrder;
  const size_t kFrameSize;
  const size_t kFrameSizeOrder;

  // Each bit indicates whether certain block should be merged or not.
  std::vector<int> is_block_splitted_;

  std::vector<FrameDescriptor*> free_lists_;
};

class UserPageAllocator {};
}  // namespace Kernel

#endif
