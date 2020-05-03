#include "frame_allocator.h"

#include "../std/algorithm.h"
#include "../std/printf.h"
#include "kernel_util.h"

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
  size_t index_in_vec = index / (8 * sizeof(T));
  return v.at(index_in_vec) & (0x1 << (index % (8 * sizeof(T))));
}

template <typename T>
constexpr int FlipBitByIndex(std::vector<T>* v, size_t index) {
  // int --> 32 bits.
  size_t index_in_vec = index / (8 * sizeof(T));
  (*v)[index_in_vec] = v->at(index_in_vec) ^ (0x1 << (index % (8 * sizeof(T))));

  // Returns the modified bit.
  return GetBitByIndex(*v, index);
}

}  // namespace

BuddyBlockAllocator::BuddyBlockAllocator(uint8_t* const start_phys_addr,
                                         int buddy_block_allocator_order,
                                         size_t frame_size)
    : start_phys_addr_(start_phys_addr),
      kBuddyBlockAllocatorOrder(buddy_block_allocator_order),
      kFrameSize(frame_size),
      kFrameSizeOrder(LargestPowerOf2DivisorOrder(frame_size)),
      need_merge_(PowerOf2(kBuddyBlockAllocatorOrder) - 1, 0),
      block_splitted_(PowerOf2(kBuddyBlockAllocatorOrder) - 1, 0),
      free_lists_(kBuddyBlockAllocatorOrder + 1, nullptr) {
  // Create the giant block that spans entire memory.
  auto* start_block = new FrameDescriptor(start_phys_addr_);
  start_block->prev = start_block;
  start_block->next = start_block;
  free_lists_[kBuddyBlockAllocatorOrder] = start_block;
}

void* BuddyBlockAllocator::GetFrame(int order) {
  // Iterate starting from freelist[order], find the empty page.
  int free_list_index = -1;
  for (int i = order; i <= kBuddyBlockAllocatorOrder; i++) {
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
  void* addr = frame_desc->page;
  delete frame_desc;

  // We have to split the memory if larger chunk is only available.
  if (free_list_index > order) {
    if (free_list_index < kBuddyBlockAllocatorOrder) {
      FlipNeedMerge(GetOffset(addr), free_list_index + 1);
    }
    Split(free_list_index, order, addr);
  } else {
    // If we are using one of the already "free" page, then we have to filp
    // "NeedMerge" bit of the containing block.
    if (order < kBuddyBlockAllocatorOrder) {
      FlipNeedMerge(GetOffset(addr), order + 1);
    }
  }

  return addr;
}

void BuddyBlockAllocator::Split(size_t free_list_index, size_t order,
                                void* addr) {
  size_t offset = GetOffset(addr);
  for (size_t i = free_list_index; i > order; i--) {
    FlipNeedMerge(offset, i);
    SetSplitted(offset, i);

    // From the splitted chunk, put "right" part to the free list.
    AddToFreeList(i - 1, offset + kFrameSize * PowerOf2(i - 1));
  }
}

void BuddyBlockAllocator::FreeFrame(void* addr) {
  size_t offset = GetOffset(addr);

  // Possible largest size of this block.
  size_t largest_possible_order =
      offset == 0 ? kBuddyBlockAllocatorOrder
                  : LargestPowerOf2DivisorOrder(offset) - kFrameSizeOrder;

  // Need to figure out actual size.
  size_t actual_order = largest_possible_order;
  for (size_t order = 1; order <= largest_possible_order; order++) {
    if (IsSplitted(offset, order)) {
      actual_order = order - 1;
      break;
    }
  }

  size_t order = actual_order + 1;
  if (order <= (size_t)kBuddyBlockAllocatorOrder) {
    FlipNeedMerge(offset, order);
  }

  for (; order <= (size_t)(kBuddyBlockAllocatorOrder); order++) {
    if (IsBothFreeOrOccupied(offset, order)) {
      // If the current block is splitted, then we have to free it.
      MergeChunk(offset, order);
    } else {
      break;
    }
  }
  AddToFreeList(/*free_list_index=*/order - 1,
                GetChunkStartOffset(offset, order - 1));
}

// "Merging" here means that the making two chunks of free block within current
// block into one whole block.
//
//  sizeof(A) + sizeof(B) == 2^order
//
//  |-----------------|----------------|
//  |                 |                |
//  |        A        |        B       |  : order
//  |                 |                |
//  |-----------------|----------------|
//
//  Becomes
//
//  |-----------------|----------------|
//  |                                  |
//  |                                  |  : order
//  |                                  |
//  |-----------------|----------------|
void BuddyBlockAllocator::MergeChunk(size_t offset, size_t order) {
  // We have to find the other part of the block from the free list.
  size_t chunk_size = kFrameSize * PowerOf2(order);
  size_t index_within_layer = offset / chunk_size;
  size_t chunk_start_offset = chunk_size * index_within_layer;

  // Newly freed block is on the left side.
  if (chunk_start_offset <= offset &&
      offset < chunk_start_offset + chunk_size / 2) {
    // Remove right block from the free list.
    auto* right_page = FindPageFromFreeList(
        order - 1, GetAddrFromOffset(chunk_start_offset + chunk_size / 2));
    RemovePageFromFreeList(order - 1, right_page);
    delete right_page;
  } else {
    auto* left_page =
        FindPageFromFreeList(order - 1, GetAddrFromOffset(chunk_start_offset));
    RemovePageFromFreeList(order - 1, left_page);
    delete left_page;
  }

  if (order < (size_t)kBuddyBlockAllocatorOrder) {
    FlipNeedMerge(offset, order + 1);
  }
  SetMerged(offset, order);
}

size_t BuddyBlockAllocator::FlipNeedMerge(size_t offset, size_t order) {
  size_t chunk_size = kFrameSize * PowerOf2(order);
  size_t index_within_layer = offset / chunk_size;

  size_t index =
      PowerOf2(kBuddyBlockAllocatorOrder - order) - 1 + index_within_layer;
  return FlipBitByIndex(&need_merge_, index);
}

bool BuddyBlockAllocator::IsBothFreeOrOccupied(size_t offset,
                                               size_t order) const {
  size_t chunk_size = kFrameSize * PowerOf2(order);
  size_t index_within_layer = offset / chunk_size;

  size_t index =
      PowerOf2(kBuddyBlockAllocatorOrder - order) - 1 + index_within_layer;
  return !GetBitByIndex(need_merge_, index);
}

void BuddyBlockAllocator::SetSplitted(size_t offset, size_t order) {
  size_t chunk_size = kFrameSize * PowerOf2(order);
  size_t index_within_layer = offset / chunk_size;

  size_t index =
      PowerOf2(kBuddyBlockAllocatorOrder - order) - 1 + index_within_layer;
  if (!GetBitByIndex(block_splitted_, index)) {
    FlipBitByIndex(&block_splitted_, index);
  }
}

void BuddyBlockAllocator::SetMerged(size_t offset, size_t order) {
  size_t chunk_size = kFrameSize * PowerOf2(order);
  size_t index_within_layer = offset / chunk_size;

  size_t index =
      PowerOf2(kBuddyBlockAllocatorOrder - order) - 1 + index_within_layer;
  if (GetBitByIndex(block_splitted_, index)) {
    FlipBitByIndex(&block_splitted_, index);
  }
}

bool BuddyBlockAllocator::IsSplitted(size_t offset, size_t order) const {
  size_t chunk_size = kFrameSize * PowerOf2(order);
  size_t index_within_layer = offset / chunk_size;

  size_t index =
      PowerOf2(kBuddyBlockAllocatorOrder - order) - 1 + index_within_layer;
  return GetBitByIndex(block_splitted_, index);
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

  // If there were only "first" in the list.
  if (free_lists_[free_list_index] == first) {
    free_lists_[free_list_index] = nullptr;
  } else if (free_lists_[free_list_index] != nullptr) {
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
    if (desc != desc->next) {
      free_lists_[free_list_index] = desc->next;
    } else {
      // If desc was the last description in the list, then we mark it as
      // nullptr.
      free_lists_[free_list_index] = nullptr;
    }
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

size_t BuddyBlockAllocator::GetChunkStartOffset(size_t offset,
                                                size_t order) const {
  size_t chunk_size = kFrameSize * PowerOf2(order);
  size_t index_within_layer = offset / chunk_size;
  return chunk_size * index_within_layer;
}

void BuddyBlockAllocator::PrintNeedMergeStatus() const {
  for (int i = kBuddyBlockAllocatorOrder; i > 0; i--) {
    for (size_t j = 0; j < PowerOf2(kBuddyBlockAllocatorOrder - i); j++) {
      if (!IsBothFreeOrOccupied(j * kFrameSize * PowerOf2(i), i)) {
        kprintf("1");
      } else {
        kprintf("0");
      }
    }
    kprintf("\n");
  }
}

void BuddyBlockAllocator::PrintSplitStatus() const {
  for (int i = kBuddyBlockAllocatorOrder; i > 0; i--) {
    for (size_t j = 0; j < PowerOf2(kBuddyBlockAllocatorOrder - i); j++) {
      if (IsSplitted(j * kFrameSize * PowerOf2(i), i)) {
        kprintf("S");
      } else {
        kprintf("M");
      }
    }
    kprintf("\n");
  }
}

void BuddyBlockAllocator::PrintFreeLists() const {
  for (size_t i = 0; i < free_lists_.size(); i++) {
    kprintf("------------- %d ------------\n", i);
    const FrameDescriptor* head = free_lists_.at(i);
    if (head != nullptr) {
      const FrameDescriptor* curr = head;
      do {
        curr->Print();
        curr = curr->next;
      } while (curr != head);
    }
  }
}

bool BuddyBlockAllocator::IsEmpty() const {
  for (size_t i = 0; i < free_lists_.size() - 1; i++) {
    if (free_lists_.at(i) != nullptr) {
      return false;
    }
  }
  return free_lists_.at(free_lists_.size() - 1) != nullptr;
}

void FrameDescriptor::Print() const {
  kprintf("Page [%x] Prev [%x] Next [%x] \n", page, prev, next);
}

UserFrameAllocator::UserFrameAllocator()
    : physical_addr_boundary_(kAllocatablePhysicalAddrStart) {
  // Create 2^13 size buddy block allocator. This spans 2^25 == 32 MB bytes of
  // the physical address space.
  allocators_.push_back(
      BuddyBlockAllocator(reinterpret_cast<uint8_t*>(physical_addr_boundary_)));

  physical_addr_boundary_ += kSingleAllocatorSize;
}

void* UserFrameAllocator::AllocateFrame(int order) {
  if (order > 13) {
    kprintf("Cannot allocate order of %d > 13 \n", order);
    return nullptr;
  }

  spin_lock_.lock();
  for (auto& allocator : allocators_) {
    void* addr = allocator.GetFrame(order);
    if (addr != nullptr) {
      spin_lock_.unlock();
      return addr;
    }
  }

  // If allocator pool is not large enough, then we should create a new one.
  allocators_.push_back(
      BuddyBlockAllocator(reinterpret_cast<uint8_t*>(physical_addr_boundary_)));
  physical_addr_boundary_ += kSingleAllocatorSize;

  auto* frame = allocators_.back().GetFrame(order);
  spin_lock_.unlock();

  return frame;
}

void UserFrameAllocator::FreeFrame(void* frame) {
  ASSERT(reinterpret_cast<uint64_t>(frame) < physical_addr_boundary_);
  size_t index =
      (reinterpret_cast<uint64_t>(frame) - kAllocatablePhysicalAddrStart) /
      kSingleAllocatorSize;
  ASSERT(index < allocators_.size());

  spin_lock_.lock();
  allocators_[index].FreeFrame(frame);
  spin_lock_.unlock();
}

}  // namespace Kernel
