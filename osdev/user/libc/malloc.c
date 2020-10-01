#include "malloc.h"

#include "printf.h"
#include "syscall.h"

#define FOUR_KB (1 << 12)
#define END_OF_LIST 0xBBBBBBBB

/*
 * Occupied Block
 * --------------------------------------------------------------------------
 * | MetaFieldFront (4 b) |  (data) .....  | MetaFieldBackOccupied (4 b)    |
 * --------------------------------------------------------------------------
 *                        |<---- size ---->|
 *
 * Free Block
 * --------------------------------------------------------------------------
 * |MetaFieldFront(4)|....|MetaFieldBackFreeList(4)|MetaFieldBackOccupied(4)|
 * --------------------------------------------------------------------------
 *                   |<----------- size ---------->|
 */

static size_t allocated_size = 0;

// Pre-defined heap start address.
static char* heap_start = (char*)0x10000004;

static size_t free_block_list_start = END_OF_LIST;

struct MetaFieldFront {
  uint32_t is_free : 1;
  uint32_t sz : 31;
};

struct MetaFieldBack {
  uint32_t is_free : 1;
  uint32_t sz : 31;
};

struct MetaFieldBackFreeList {
  uint32_t prev_free;
  uint32_t next_free;
};

#define META_BLOCK_SZ \
  (sizeof(struct MetaFieldFront) + sizeof(struct MetaFieldBack))

size_t RoundUpToMultipleOfFourKb(size_t bytes) {
  if (bytes % FOUR_KB == 0) {
    return bytes;
  }
  return ((bytes / FOUR_KB) + 1) * FOUR_KB;
}

size_t RoundUpToMultipleOf8(size_t bytes) {
  if (bytes % 8 == 0) {
    return bytes;
  }
  return ((bytes / 8) + 1) * 8;
}

void SetMetaField(int is_free, uint32_t sz, uint32_t* meta) {
  *meta = (is_free | (sz << 1));
}

struct MetaFieldFront* GetMetaFieldFront(size_t current) {
  return (struct MetaFieldFront*)(heap_start + current);
}

struct MetaFieldFront* GetRightNeighborMetaFront(size_t offset) {
  struct MetaFieldFront* front = GetMetaFieldFront(offset);
  return GetMetaFieldFront(offset + sizeof(struct MetaFieldFront) + front->sz +
                           sizeof(struct MetaFieldBack));
}

size_t GetRightNeighborOffset(size_t offset) {
  struct MetaFieldFront* front = GetMetaFieldFront(offset);
  return offset + sizeof(struct MetaFieldFront) + front->sz +
         sizeof(struct MetaFieldBack);
}

struct MetaFieldBack* GetMetaFieldBack(size_t current) {
  return (struct MetaFieldBack*)(heap_start + current);
}

// Return the start block.
size_t GetLeftNeighborOffset(size_t offset) {
  struct MetaFieldBack* back =
      GetMetaFieldBack(offset - sizeof(struct MetaFieldBack));
  return offset - META_BLOCK_SZ - back->sz;
}

struct MetaFieldBack* GetMetaFieldBackFromStart(size_t offset) {
  struct MetaFieldFront* front = GetMetaFieldFront(offset);
  return GetMetaFieldBack(offset + sizeof(struct MetaFieldFront) + front->sz);
}

struct MetaFieldBackFreeList* GetMetaFieldBackFreeList(
    size_t free_list_offset) {
  return (struct MetaFieldBackFreeList*)(heap_start + free_list_offset);
}

struct MetaFieldBackFreeList* GetMetaFieldBackFreeListFromStart(size_t offset) {
  struct MetaFieldFront* front = GetMetaFieldFront(offset);
  return GetMetaFieldBackFreeList(offset + sizeof(struct MetaFieldFront) +
                                  front->sz -
                                  sizeof(struct MetaFieldBackFreeList));
}

// Add to the free list.
void AddToFreeList(size_t offset) {
  struct MetaFieldBackFreeList* back =
      GetMetaFieldBackFreeListFromStart(offset);

  back->next_free = free_block_list_start;
  back->prev_free = END_OF_LIST;

  if (free_block_list_start != END_OF_LIST) {
    struct MetaFieldBackFreeList* next_free =
        GetMetaFieldBackFreeListFromStart(free_block_list_start);
    next_free->prev_free = offset;
  }

  free_block_list_start = offset;
}

// Remove free block from the list.
void RemoveFromFreeList(size_t offset) {
  struct MetaFieldBackFreeList* free_list =
      GetMetaFieldBackFreeListFromStart(offset);
  if (free_block_list_start == offset) {
    free_block_list_start = free_list->next_free;
  }

  if (free_list->prev_free != END_OF_LIST) {
    struct MetaFieldBackFreeList* prev_free =
        GetMetaFieldBackFreeListFromStart(free_list->prev_free);
    prev_free->next_free = free_list->next_free;
  }

  if (free_list->next_free != END_OF_LIST) {
    struct MetaFieldBackFreeList* next_free =
        GetMetaFieldBackFreeListFromStart(free_list->next_free);
    next_free->prev_free = free_list->prev_free;
  }
}

void MarkOccupied(size_t offset) {
  struct MetaFieldFront* front = GetMetaFieldFront(offset);
  front->is_free = 0;

  // Rewire prev and next free blocks.
  struct MetaFieldBack* back = GetMetaFieldBackFromStart(offset);
  back->is_free = 0;

  RemoveFromFreeList(offset);
}

void MergeTwoBlocks(size_t left, size_t right) {
  // 1. Remove left block from the list.
  RemoveFromFreeList(left);

  // 2. Modify the size of the left chunk.
  struct MetaFieldFront* left_front = GetMetaFieldFront(left);
  struct MetaFieldFront* right_front = GetMetaFieldFront(right);
  left_front->sz += (right_front->sz + META_BLOCK_SZ);

  // Since left's size has updated, we can get the correct Back.
  struct MetaFieldBack* back = GetMetaFieldBackFromStart(left);
  back->sz = left_front->sz;

  // This free list is actually right's free list.
  struct MetaFieldBackFreeList* free_list =
      GetMetaFieldBackFreeListFromStart(left);
  if (free_list->prev_free != END_OF_LIST) {
    struct MetaFieldBackFreeList* prev_free =
        GetMetaFieldBackFreeListFromStart(free_list->prev_free);
    prev_free->next_free = left;
  }

  if (free_list->next_free != END_OF_LIST) {
    struct MetaFieldBackFreeList* next_free =
        GetMetaFieldBackFreeListFromStart(free_list->next_free);
    next_free->prev_free = left;
  }

  if (free_block_list_start == right) {
    free_block_list_start = left;
  }
}

void MarkFree(size_t offset) {
  struct MetaFieldFront* front = GetMetaFieldFront(offset);
  struct MetaFieldBack* back = GetMetaFieldBackFromStart(offset);

  front->is_free = 1;
  back->is_free = 1;

  AddToFreeList(offset);

  // Now consider merging nearby free blocks (if exist).
  size_t right_neighbor = GetRightNeighborOffset(offset);
  if (right_neighbor < allocated_size) {
    struct MetaFieldFront* right_neighbor_front =
        GetMetaFieldFront(right_neighbor);
    if (right_neighbor_front->is_free) {
      MergeTwoBlocks(offset, right_neighbor);
    }
  }

  if (offset > 0) {
    size_t left_neighbor = GetLeftNeighborOffset(offset);
    struct MetaFieldFront* left_neighbor_front =
        GetMetaFieldFront(left_neighbor);
    if (left_neighbor_front->is_free) {
      MergeTwoBlocks(left_neighbor, offset);
    }
  }
}

size_t FindFreeBlock(size_t sz) {
  size_t current = free_block_list_start;
  while (current != END_OF_LIST) {
    struct MetaFieldFront* front = GetMetaFieldFront(current);
    if (front->sz >= sz) {
      return current;
    }
    struct MetaFieldBackFreeList* back =
        GetMetaFieldBackFreeListFromStart(current);
    current = back->next_free;
  }

  return -1;
}

// Split the block.
// ---------------------------
// |    L    |      R        |
// ---------------------------
//
void SplitFreeBlock(size_t offset, size_t bytes) {
  struct MetaFieldFront* front = GetMetaFieldFront(offset);
  // Check whether the size of the free block is large enough to be splitted.
  if (front->sz <
      bytes + META_BLOCK_SZ + sizeof(struct MetaFieldBackFreeList)) {
    return;
  }

  int new_free_block_size = front->sz - (bytes + META_BLOCK_SZ);
  front->sz = bytes;

  struct MetaFieldBack* back = GetMetaFieldBackFromStart(offset);
  back->is_free = 1;
  back->sz = bytes;

  size_t neighbor = GetRightNeighborOffset(offset);

  // Adjust current free list to point Right block if it were already pointing
  // the offset.
  if (free_block_list_start == offset) {
    free_block_list_start = neighbor;
  }

  struct MetaFieldFront* splitted_front = GetMetaFieldFront(neighbor);

  splitted_front->is_free = 1;
  splitted_front->sz = new_free_block_size;

  struct MetaFieldBack* splitted_back = GetMetaFieldBackFromStart(neighbor);
  splitted_back->is_free = 1;
  splitted_back->sz = new_free_block_size;

  // Need to adjust Right block's size to make sure Left block to be added to
  // free list properly.
  AddToFreeList(offset);

  struct MetaFieldBackFreeList* splitted_free_list =
      GetMetaFieldBackFreeListFromStart(neighbor);
  if (splitted_free_list->prev_free != END_OF_LIST) {
    struct MetaFieldBackFreeList* prev_free =
        GetMetaFieldBackFreeListFromStart(splitted_free_list->prev_free);
    prev_free->next_free = neighbor;
  }

  if (splitted_free_list->next_free != END_OF_LIST) {
    struct MetaFieldBackFreeList* next_free =
        GetMetaFieldBackFreeListFromStart(splitted_free_list->next_free);
    next_free->prev_free = neighbor;
  }
}

void InitMalloc() {
  if (allocated_size != 0) {
    return;
  }

  sbrk(FOUR_KB);
  allocated_size = FOUR_KB - 4;

  struct MetaFieldFront* front = GetMetaFieldFront(0);
  front->sz = FOUR_KB - 4 - META_BLOCK_SZ;

  struct MetaFieldBack* back = GetMetaFieldBackFromStart(0);
  back->sz = front->sz;

  MarkFree(0);
}

void* malloc(unsigned long int bytes) {
  if (bytes == 0) {
    return NULL;
  }

  if (allocated_size == 0) {
    InitMalloc();
  }

  bytes = RoundUpToMultipleOf8(bytes);

  // Try finding a free block.
  int free_block_offset = FindFreeBlock(bytes);
  if (free_block_offset != -1) {
    SplitFreeBlock(free_block_offset, bytes);
    MarkOccupied(free_block_offset);
    return heap_start + free_block_offset + 4;
  }

  // If unable to find, then fetch more memory.
  size_t needed_mem = RoundUpToMultipleOfFourKb(bytes + META_BLOCK_SZ);

  // Add more memory.
  sbrk(needed_mem);

  size_t new_free_mem = allocated_size;

  allocated_size += needed_mem;

  // Add new "Free" node from acquired memory.
  struct MetaFieldFront* front = GetMetaFieldFront(new_free_mem);
  front->sz = needed_mem - META_BLOCK_SZ;

  struct MetaFieldBack* back = GetMetaFieldBackFromStart(new_free_mem);
  back->sz = front->sz;

  MarkFree(new_free_mem);

  // If left block has been merged, then we have to update new_free_mem.
  struct MetaFieldBack* merged_back =
      GetMetaFieldBack(allocated_size - sizeof(struct MetaFieldBack));
  new_free_mem = allocated_size - META_BLOCK_SZ - merged_back->sz;

  SplitFreeBlock(new_free_mem, bytes);
  MarkOccupied(new_free_mem);

  return heap_start + new_free_mem + 4;
}

void* realloc(void* ptr, unsigned long int bytes) {
  if (ptr == 0) {
    return malloc(bytes);
  }

  size_t offset = (char*)(ptr)-heap_start - 4;
  struct MetaFieldFront* front = GetMetaFieldFront(offset);
  if (front->sz >= bytes) {
    return ptr;
  }

  free(ptr);
  return malloc(bytes);
}

void* calloc(unsigned long int num, unsigned long int size) {
  char* data = (char*)malloc(num * size);
  if (data == NULL) {
    return NULL;
  }

  for (size_t i = 0; i < num * size; i++) {
    data[i] = 0;
  }

  return data;
}

void free(void* mem) {
  if (mem == 0) {
    return;
  }
  size_t offset = (char*)(mem)-heap_start - 4;
  MarkFree(offset);
}

void __print_free_list() {
  size_t current = free_block_list_start;
  while (current != END_OF_LIST) {
    struct MetaFieldFront* front = GetMetaFieldFront(current);
    struct MetaFieldBack* back = GetMetaFieldBackFromStart(current);
    struct MetaFieldBackFreeList* free_list =
        GetMetaFieldBackFreeListFromStart(current);

    int sanity_check =
        (front->sz == back->sz) && (front->is_free == back->is_free);
    printf("sz [%x] [Is free? : %d] [Prev : %x] [Next %x] [Sanity : %d]\n",
           front->sz, front->is_free, free_list->prev_free,
           free_list->next_free, sanity_check);
    current = free_list->next_free;
  }
}

void __malloc_show_status() {
  size_t current = 0;
  printf("Free list start : %x \n", free_block_list_start);
  while (current < allocated_size) {
    struct MetaFieldFront* front = GetMetaFieldFront(current);
    struct MetaFieldBack* back = GetMetaFieldBackFromStart(current);

    printf("[Offset : %x] [Free? %d] Size : %x [=%x] \n", current,
           front->is_free, front->sz, back->sz);

    if (front->is_free) {
      struct MetaFieldBackFreeList* free_list =
          GetMetaFieldBackFreeListFromStart(current);
      printf("Free list : prev %lx next : %lx \n", free_list->prev_free,
             free_list->next_free);
    }
    current += (front->sz + META_BLOCK_SZ);
  }
}

// Iterate entire memory block.
bool __malloc_sanity_check() {
  size_t current = 0;
  while (current < allocated_size) {
    struct MetaFieldFront* front = GetMetaFieldFront(current);
    struct MetaFieldBack* back = GetMetaFieldBackFromStart(current);

    if (front->sz != back->sz) {
      printf("Malloc mismatch at %lx [front sz : %x] [back sz : %x] \n",
             current, front->sz, back->sz);
      return false;
    } else if (front->is_free != back->is_free) {
      printf("Malloc mismatch at %lx [front free? %x] [back free? %x] \n",
             current, front->is_free, back->is_free);
      return false;
    }
    current += (front->sz + META_BLOCK_SZ);
  }

  return true;
}
