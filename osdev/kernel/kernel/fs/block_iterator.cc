#include "block_iterator.h"

namespace Kernel {

Block BlockIterator::operator*() {
  // First check the cache. If it is stale, then it will read from the disk.
  FillCache();

  // Now returns the cache.
  Block block;
  block.CopyFrom(cached_blocks_[current_index_.CurrentDepth()].data);
  return block;
}

void BlockIterator::FillCache() {
  size_t cache_fill_start = 0;
  for (cache_fill_start = 0; cache_fill_start <= current_index_.CurrentDepth();
       cache_fill_start++) {
    if (cached_blocks_[cache_fill_start].index !=
        current_index_[cache_fill_start]) {
      break;
    }
  }

  // We should start filling from cache_fill_start.
  for (size_t current_fill = cache_fill_start;
       current_fill <= current_index_.CurrentDepth(); current_fill++) {
    cached_blocks_[current_fill].index = current_index_[current_fill];

    // If this is a first depth, we can just read block address from inode.
    if (current_fill == 0) {
      cached_blocks_[0].block_addr = inode_->block[current_index_[0]];
      GetFromBlockId(&cached_blocks_[0].data, cached_blocks_[0].block_addr);
    } else {
      // We should read the block adderss from the previous block.
      cached_blocks_[current_fill].block_addr =
          cached_blocks_[current_fill - 1].data[current_index_[current_fill]];
      GetFromBlockId(&cached_blocks_[current_fill].data,
                     cached_blocks_[current_fill].block_addr);
    }
  }
}

int BlockIterator::GetBlockId(size_t depth) {
  if (depth > current_index_.CurrentDepth()) {
    return -1;
  }

  int block_id = -1;
  for (size_t i = 0; i <= depth; i++) {
    cached_blocks_[i].index = current_index_[i];
    if (i == 0) {
      block_id = inode_->block[current_index_[0]];

      cached_blocks_[0].block_addr = inode_->block[current_index_[0]];
      GetFromBlockId(&cached_blocks_[0].data, cached_blocks_[0].block_addr);
    } else {
      cached_blocks_[i].block_addr =
          cached_blocks_[i - 1].data[current_index_[i]];
      GetFromBlockId(&cached_blocks_[i].data, cached_blocks_[i].block_addr);
      block_id = cached_blocks_[i].block_addr;
    }
  }

  return block_id;
}

Block BlockIterator::GetBlockFromDepth(size_t depth) {
  if (depth > current_index_.CurrentDepth()) {
    PANIC();
    return Block();
  }

  for (size_t i = 0; i <= depth; i++) {
    cached_blocks_[i].index = current_index_[i];
    if (i == 0) {
      cached_blocks_[0].block_addr = inode_->block[current_index_[0]];
      GetFromBlockId(&cached_blocks_[0].data, cached_blocks_[0].block_addr);
    } else {
      cached_blocks_[i].block_addr =
          cached_blocks_[i - 1].data[current_index_[i]];
      GetFromBlockId(&cached_blocks_[i].data, cached_blocks_[i].block_addr);
    }
  }

  Block block;
  block.CopyFrom(cached_blocks_[depth].data);
  return block;
}

void BlockIterator::SetBlockId(size_t depth, size_t block_id) {
  if (depth > current_index_.CurrentDepth()) {
    return;
  }

  for (size_t i = 0; i <= depth; i++) {
    cached_blocks_[i].index = current_index_[i];
    if (i == 0) {
      inode_->block[current_index_[0]] = block_id;

      cached_blocks_[0].block_addr = inode_->block[current_index_[0]];
      GetFromBlockId(&cached_blocks_[0].data, cached_blocks_[0].block_addr);
    } else {
      if (i == depth) {
        cached_blocks_[i].block_addr = block_id;
      } else {
        cached_blocks_[i].block_addr =
            cached_blocks_[i - 1].data[current_index_[i]];
      }
      GetFromBlockId(&cached_blocks_[i].data, cached_blocks_[i].block_addr);
    }
  }
}

void BlockIterator::Print() const {
  kprintf("[");
  for (size_t i = 0; i <= current_index_.CurrentDepth(); i++) {
    kprintf("%d ", current_index_[i]);
  }
  kprintf("]");
}

}  // namespace Kernel
