#include "block_indices.h"
#include "ext2.h"

namespace Kernel {

constexpr size_t kBlockSize = 1024;
constexpr static size_t kMaxBlockEntryInBlock = kBlockSize / sizeof(uint32_t);

using Block = std::array<uint8_t, kBlockSize>;

class BlockIterator {
 public:
  BlockIterator(Ext2Inode* inode) : inode_(inode) {}

  BlockIterator& operator++() {
    ++current_index_;
    current_pos += kBlockSize;

    return *this;
  }

  const BlockIndices& Index() const { return current_index_; }

  Block operator*();

  bool operator==(const BlockIterator& b) const {
    return current_index_ == b.current_index_;
  }

  bool operator!=(const BlockIterator& b) const { return !operator==(b); }

  size_t Pos() const { return current_pos; }

  void SetOffset(size_t offset) {
    current_pos = (offset / kBlockSize) * kBlockSize;
    current_index_ = BlockIndices::CreateBlockIndiceFromOffset(offset);
  }

  // Get block id at depth.
  // E.g index: [13, 4, 5], asked depth = 2
  // inode.block[13] = 1234
  // Block[1234] --> 4 th entry = 2345
  // Block[2345] --> 5 th entry = 3456
  // Returns 3456
  int GetBlockId(size_t depth);
  Block GetBlockFromDepth(size_t depth);
  void SetBlockId(size_t depth, size_t block_id);

  // Get the block id of the current data block.
  int GetDataBlockID() { return GetBlockId(current_index_.CurrentDepth()); }

  void Print() const;

  static void SetNthEntryAtAddressBlock(Block* block, size_t index,
                                        size_t block_id) {
    uint32_t* addr_block = reinterpret_cast<uint32_t*>(block->data());
    addr_block[index] = block_id;
  }

 private:
  void FillCache();

  Ext2Inode* inode_;
  size_t current_pos = 0;

  BlockIndices current_index_;

  // Block that only contains the block address. Used by doubly and trebly
  // address blocks.
  using AddressBlock = std::array<uint32_t, kMaxBlockEntryInBlock>;

  struct CachedBlock {
    uint32_t block_addr = -1;
    uint32_t index = -1;
    AddressBlock data;
  };

  std::array<CachedBlock, 4> cached_blocks_;
};
}  // namespace Kernel

