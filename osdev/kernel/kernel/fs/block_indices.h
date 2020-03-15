#include "../../std/array.h"
#include "../../std/printf.h"
#include "../../std/types.h"
#include "../kernel_util.h"

namespace Kernel {
class BlockIndices {
 public:
  static BlockIndices CreateBlockIndiceFromOffset(size_t offset);

  BlockIndices() {
    current_depth_ = 0;
    block_index_.fill(0);
  }

  BlockIndices(int* indices, size_t current_depth)
      : current_depth_(current_depth) {
    block_index_.fill(0);

    for (size_t i = 0; i <= current_depth; i++) {
      block_index_[i] = indices[i];
    }
  }

  BlockIndices(const BlockIndices& b) {
    current_depth_ = b.current_depth_;
    for (size_t i = 0; i < 4; i++) {
      block_index_[i] = b.block_index_.at(i);
    }
  }

  BlockIndices& operator=(const BlockIndices& b) {
    current_depth_ = b.current_depth_;
    for (size_t i = 0; i < 4; i++) {
      block_index_[i] = b.block_index_.at(i);
    }

    return *this;
  }

  size_t operator[](size_t i) const {
    if (i > current_depth_) {
      PANIC();
    }
    return block_index_.at(i);
  }

  size_t CurrentDepth() const { return current_depth_; }

  BlockIndices& operator++() {
    RecursiveIndexIncrease(current_depth_);
    return *this;
  }
  bool operator==(const BlockIndices& b) const {
    if (current_depth_ != b.current_depth_) {
      return false;
    }
    for (size_t i = 0; i <= current_depth_; i++) {
      if (block_index_.at(i) != b.block_index_.at(i)) {
        return false;
      }
    }
    return true;
  }
  bool operator!=(const BlockIndices& b) const { return !(operator==(b)); }

  void Print() const;
 private:
  void RecursiveIndexIncrease(int current_depth);

  // 0 : direct block.
  // 1 : single indirect block.
  // 2 : doubly indirect block.
  // 3 : triply indirect block.
  size_t current_depth_;

  // Index of each block at each level. E.g if this block is double indirect
  // block, then it can be {13, 4, 5}
  //
  // File Inode: block[13] = 10.
  // At block 10, we are at block[4] = 30.
  // At block 30, we are at block[5] = 50.
  std::array<uint32_t, 4> block_index_;
};

}  // namespace Kernel
