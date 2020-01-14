#include "paging.h"

namespace Kernel {
namespace {
// E.g 12 --> 4. 96 --> 32.
constexpr size_t LargestPowerOf2Divisor(size_t x) { return x & (-x); }

// E.g 12 --> 2. 96 --> 5.
constexpr size_t LargestPowerOf2DivisorOrder(size_t x) {
  return __builtin_ctz(LargestPowerOf2Divisor(x));
}

constexpr size_t PowerOf2(size_t x) { return 1 << x; }

template <typename T>
constexpr int GetBitByIndex(const std::vector<T>& v, size_t index) {
  // int --> 32 bits.
  size_t index_in_vec = index / sizeof(T);
  return v.at(index_in_vec) & (0x1 << (index % sizeof(T)));
}

template <typename T>
constexpr int FlipBitByIndex(std::vector<T>* v, size_t index) {
  // int --> 32 bits.
  size_t index_in_vec = index / sizeof(T);
  (*v)[index_in_vec] = v->at(index_in_vec) ^ (0x1 << (index % sizeof(T)));

  // Returns the modified bit.
  return GetBitByIndex(*v, index);
}

}  // namespace

BuddyBlockAllocator::BuddyBlockAllocator(uint8_t* const start_phys_addr)
    : start_phys_addr_(start_phys_addr) {
  // Initialize free list.
  for (size_t i = 0; i < kBuddyBlockAllocatorOrder; i++) {
    free_lists_[i] = nullptr;
  }

  // Create the giant block that spans entire memory.
  auto* start_block = new FrameDescriptor(start_phys_addr_);
  start_block->prev = start_block;
  start_block->next = start_block;
  free_lists_[kBuddyBlockAllocatorOrder] = start_block;

  is_block_splitted_.reserve(PowerOf2(kBuddyBlockAllocatorOrder) - 1);
  std::fill(is_block_splitted_.begin(), is_block_splitted_.end(), 0);

  free_leaf_info_.reserve(PowerOf2(kBuddyBlockAllocatorOrder));
  std::fill(free_leaf_info_.begin(), free_leaf_info_.end(), 0);
}

void* BuddyBlockAllocator::GetFrame(size_t order) {
  // Iterate starting from freelist[order], find the empty page.
  size_t free_list_index = -1;
  for (size_t i = order; i <= kBuddyBlockAllocatorOrder; i++) {
    if (free_lists_[i] != nullptr) {
      free_list_index = i;
      break;
    }
  }

  // No free memory available for that size.
  if (free_list_index == -1) {
    return nullptr;
  }

  // Remove current chunk from free list.
  auto* frame_desc = RemoveFirstFromFreeList(free_list_index);

  // We have to split the memory if larger chunk is only available.
  if (free_list_index > order) {
    Split(free_list_index, order);
  }

  return frame_desc->page;
}

void BuddyBlockAllocator::FreeFrame(void* addr) {
  size_t offset = GetOffset(addr);

  // We need to find the size of the block.
  size_t largest_order =
      LargestPowerOf2DivisorOrder(offset) - kFrameSizeOrder + 1;

  for (size_t order = 1; order <= largest_order; order++) {
    if (IsBlockSplitted(offset, order)) {
      // If the current block is splitted, then we have to free it.
      MergeChunk(order, offset);
    }
  }
}

void BuddyBlockAllocator::Split(size_t free_list_index, size_t order) {
  FrameDescriptor* frame_desc = free_lists_[free_list_index];
  size_t offset = GetOffset(frame_desc->page);

  // Get the bit index from is_block_splitted.
  for (size_t i = free_list_index; i > order; i++) {
    FlipBlockSplitted(offset, i);

    // From the splitted chunk, put "right" part to the free list.
    AddToFreeList(i - 1, offset + kFrameSize * PowerOf2(i - 1));
  }
}

void BuddyBlockAllocator::MergeChunk(size_t order, size_t offset) {
  //
}

size_t BuddyBlockAllocator::FlipBlockSplitted(size_t offset, size_t order) {
  size_t chunk_size = kFrameSize * PowerOf2(order);
  size_t index_within_layer = offset / chunk_size;

  size_t index =
      PowerOf2(kBuddyBlockAllocatorOrder - order) - 1 + index_within_layer;
  return FlipBitByIndex(&is_block_splitted_, index);
}

bool BuddyBlockAllocator::IsBlockSplitted(size_t offset, size_t order) const {
  size_t chunk_size = kFrameSize * PowerOf2(order);
  size_t index_within_layer = offset / chunk_size;

  size_t index =
      PowerOf2(kBuddyBlockAllocatorOrder - order) - 1 + index_within_layer;
  return GetBitByIndex(is_block_splitted_, index);
}

void BuddyBlockAllocator::AddToFreeList(size_t free_list_index, size_t offset) {
  auto* frame_desc = new FrameDescriptor(start_phys_addr_ + offset);
  if (free_lists_[free_list_index] != nullptr) {
    auto* current_head = free_lists_[free_list_index];
    frame_desc->next = current_head;
    frame_desc->prev = current_head->prev;

    current_head->prev->next = frame_desc;
    current_head->prev = frame_desc;
  } else {
    free_lists_[free_list_index] = frame_desc;
    frame_desc->next = frame_desc;
    frame_desc->prev = frame_desc;
  }
}

// Returns the first element from the free list.
FrameDescriptor* BuddyBlockAllocator::RemoveFirstFromFreeList(
    size_t free_list_index) {
  auto* first = free_lists_[free_list_index];
  free_lists_[free_list_index] = first->next;

  if (free_lists_[free_list_index] != nullptr) {
    auto* new_first = free_lists_[free_list_index];
    new_first->prev = first->prev;
    first->prev->next = new_first;
  }

  return first;
}

// Remove the page from free_list.
void BuddyBlockAllocator::RemovePageFromFreeList(size_t free_list_index,
                                                 FrameDescriptor* desc) {
  // Wire previous node and next node together.
  if (desc->prev != nullptr) {
    desc->prev->next = desc->next;
  }
  if (desc->next != nullptr) {
    desc->next->prev = desc->prev;
  }

  // Move the head to point next if the head is getting removed.
  if (free_lists_[free_list_index] == desc) {
    free_lists_[free_list_index] = desc->next;
  }
}

// Find page_addr from free_list
FrameDescriptor* BuddyBlockAllocator::FindPageFromFreeList(
    size_t free_list_index, void* page_addr) {
  FrameDescriptor* start = free_lists_[free_list_index];
  FrameDescriptor* curr = start;

  if (curr == nullptr) {
    return nullptr;
  }
  do {
    if (curr->page == page_addr) {
      return curr;
    }
    curr = curr->next;
  } while (start != curr && curr != nullptr);
  return nullptr;
}
}  // namespace Kernel
