#include "block_indices.h"

namespace Kernel {
namespace {

constexpr size_t kBlockSize = 1024;
constexpr size_t kMaxBlockEntryInBlock = kBlockSize / sizeof(uint32_t);

}  // namespace

void BlockIndices::RecursiveIndexIncrease(int current_depth) {
  if (current_depth == 0) {
    if (block_index_[0] >= 11) {
      current_depth_++;
    }
    block_index_[0]++;

    return;
  }

  if (block_index_[current_depth] == kMaxBlockEntryInBlock - 1) {
    block_index_[current_depth] = 0;
    RecursiveIndexIncrease(current_depth - 1);
  } else {
    block_index_[current_depth]++;
  }
}

BlockIndices BlockIndices::CreateBlockIndiceFromOffset(size_t offset) {
  constexpr size_t direct_block_limit = kBlockSize * 12;
  constexpr size_t single_block_limit =
      direct_block_limit + kBlockSize * kMaxBlockEntryInBlock;
  constexpr size_t double_block_limit =
      single_block_limit +
      kBlockSize * kMaxBlockEntryInBlock * kMaxBlockEntryInBlock;
  constexpr size_t triple_block_limit =
      double_block_limit + kBlockSize * kMaxBlockEntryInBlock *
                               kMaxBlockEntryInBlock * kMaxBlockEntryInBlock;
  constexpr size_t single_block_entry_offset =
      kBlockSize * kMaxBlockEntryInBlock;
  constexpr size_t double_block_entry_offset =
      kBlockSize * kMaxBlockEntryInBlock * kMaxBlockEntryInBlock;

  int block_index[4], current_depth = 0;

  if (offset < direct_block_limit) {
    current_depth = 0;
    block_index[0] = offset / kBlockSize;
  } else if (offset < single_block_limit) {
    current_depth = 1;
    block_index[0] = 12;
    block_index[1] = (offset - direct_block_limit) / kBlockSize;
  } else if (offset < double_block_limit) {
    current_depth = 2;
    block_index[0] = 13;
    block_index[1] = (offset - single_block_limit) / single_block_entry_offset;
    block_index[2] = (offset - single_block_limit -
                      block_index[1] * single_block_entry_offset) /
                     kBlockSize;
  } else if (offset < triple_block_limit) {
    current_depth = 3;
    block_index[0] = 14;
    block_index[1] = (offset - single_block_limit) / double_block_entry_offset;

    size_t second_base = offset - single_block_limit -
                         block_index[1] * double_block_entry_offset;
    block_index[2] = second_base / single_block_entry_offset;

    size_t third_base =
        second_base - block_index[2] * single_block_entry_offset;
    block_index[3] = third_base / kBlockSize;
  } else {
    kprintf("Offset too large! %x \n", offset);
  }

  return BlockIndices(block_index, current_depth);
}

void BlockIndices::Print() const {
  kprintf("Block index : ");
  for (size_t i = 0; i <= current_depth_; i ++) {
    kprintf("%x ", block_index_.at(i));
  }
  kprintf("\n");
}


}  // namespace Kernel
