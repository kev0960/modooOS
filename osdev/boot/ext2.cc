#include "ext2.h"
#include "algorithm.h"
#include "array.h"
#include "ata.h"
#include "kmalloc.h"
#include "printf.h"
#include "string.h"

namespace Kernel {
constexpr size_t kBlockSize = 1024;
constexpr size_t kMaxBlockEntryInBlock = kBlockSize / sizeof(uint32_t);

using Block = std::array<uint8_t, kBlockSize>;

namespace {

void ParseInodeMode(uint16_t mode) {
  kprintf("Mode :: ");
  const char* file_formats[] = {
      "fifo", "character device", "dir", "block device", "file", "sym", "sock"};
  uint16_t file_format_modes[] = {0x1000, 0x2000, 0x4000, 0x6000,
                                  0x8000, 0xA000, 0xC000};

  for (int i = 0; i < 7; i++) {
    if ((mode & file_format_modes[i]) == file_format_modes[i]) {
      kprintf("%s ", file_formats[i]);
    }
  }

  const char* access[] = {"r", "w", "x", "r", "w", "x", "r", "w", "x"};
  uint16_t access_modes[] = {0x100, 0x80, 0x40, 0x20, 0x10, 0x8, 0x4, 0x2, 0x1};
  for (int i = 0; i < 9; i++) {
    if ((mode & access_modes[i]) == access_modes[i]) {
      kprintf("%s", access[i]);
    } else {
      kprintf("-");
    }
  }
  kprintf("\n");
}

template <typename T>
void GetFromBlockId(T* t, size_t block_id) {
  // sector size is 512 bytes. That means, 1 block spans 2 sectors.
  ATADriver::GetATADriver().Read(t, 2 * block_id);
}

template <typename T>
void GetArrayFromBlockId(T* t, size_t num, size_t block_id) {
  // sector size is 512 bytes. That means, 1 block spans 2 sectors.
  ATADriver::GetATADriver().Read(reinterpret_cast<uint8_t*>(t), sizeof(T) * num,
                                 2 * block_id);
}

[[maybe_unused]] void PrintInodeInfo(const Ext2Inode& inode) {
  kprintf("Inode Block Info ----------- \n");
  kprintf("Size (%x) Created at (%x) Num blocks (%x) ", inode.size, inode.ctime,
          inode.blocks);
  ParseInodeMode(inode.mode);
  for (size_t i = 0; i < min((uint16_t)15, inode.blocks); i++) {
    if (inode.block[i]) {
      kprintf("[%x] ", inode.block[i]);
    }
  }
  kprintf("\n");
}

template <typename T>
T ReadAndAdvance(uint8_t*& buf) {
  T t = *(T*)(buf);
  buf += sizeof(T);
  return t;
}

[[maybe_unused]] void ParseDirectory(uint8_t* buf, int num_read) {
  int current = 0;
  while (current < num_read) {
    uint8_t* prev = buf;

    // Read inode number.
    uint32_t inode = ReadAndAdvance<uint32_t>(buf);

    // Read total entry size.
    uint16_t entry_size = ReadAndAdvance<uint16_t>(buf);

    if (entry_size == 0) break;

    // Read name length.
    uint8_t name_len = ReadAndAdvance<uint8_t>(buf);

    // Read type indicator.
    uint8_t file_type = ReadAndAdvance<uint8_t>(buf);

    kprintf("Inode : (%x) Entry size : (%x) Name Len : (%x) File type : (%x) ",
            inode, entry_size, name_len, file_type);
    for (size_t i = 0; i < name_len; i++) {
      kprintf("%c", buf[i]);
    }
    kprintf("\n");
    buf = prev + entry_size;
    current += entry_size;
  }
}

class BlockIterator {
 public:
  BlockIterator(const Ext2Inode& inode) : inode_(inode), current_depth_(0) {
    block_index_.fill(0);
  }

  BlockIterator& operator++() {
    RecursiveIndexIncrease(current_depth_);
    current_pos += kBlockSize;

    return *this;
  }

  Block operator*() {
    // First check the cache. If it is stale, then it will read from the disk.
    FillCache();

    // Now returns the cache.
    Block block;
    block.CopyFrom(cached_blocks_[current_depth_].data);
    kprintf("%d %d %d %d \n", block_index_[0], block_index_[1], block_index_[2],
            block_index_[3]);
    return block;
  }

  size_t Pos() const { return current_pos; }

  std::array<uint32_t, 4> GetBlockIndex() const { return block_index_; }

  void SetOffset(size_t offset) {
    current_pos = (offset / kBlockSize) * kBlockSize;

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

    if (offset < direct_block_limit) {
      current_depth_ = 0;
      block_index_[0] = offset / kBlockSize;
    } else if (offset < single_block_limit) {
      current_depth_ = 1;
      block_index_[0] = 12;
      block_index_[1] = (offset - direct_block_limit) / kBlockSize;
    } else if (offset < double_block_limit) {
      current_depth_ = 2;
      block_index_[0] = 13;
      block_index_[1] =
          (offset - single_block_limit) / single_block_entry_offset;
      block_index_[2] = (offset - single_block_limit -
                         block_index_[1] * single_block_entry_offset) /
                        kBlockSize;
    } else if (offset < triple_block_limit) {
      current_depth_ = 3;
      block_index_[0] = 14;
      block_index_[1] =
          (offset - single_block_limit) / double_block_entry_offset;

      size_t second_base = offset - single_block_limit -
                           block_index_[1] * double_block_entry_offset;
      block_index_[2] = second_base / single_block_entry_offset;

      size_t third_base =
          second_base - block_index_[2] * single_block_entry_offset;
      block_index_[3] = third_base / kBlockSize;
    } else {
      kprintf("Offset too large! %x \n", offset);
    }
  }

 private:
  void FillCache() {
    size_t cache_fill_start = 0;
    for (cache_fill_start = 0; cache_fill_start <= current_depth_;
         cache_fill_start++) {
      if (cached_blocks_[cache_fill_start].block_addr !=
          block_index_[cache_fill_start]) {
        break;
      }
    }

    // We should start filling from cache_fill_start.
    for (size_t current_fill = cache_fill_start; current_fill <= current_depth_;
         current_fill++) {
      // If this is a first depth, we can just read block address from inode.
      if (current_fill == 0) {
        cached_blocks_[0].block_addr = inode_.block[block_index_[0]];
        GetFromBlockId(&cached_blocks_[0].data, cached_blocks_[0].block_addr);
      } else {
        // We should read the block adderss from the previous block.
        cached_blocks_[current_fill].block_addr =
            cached_blocks_[current_fill - 1].data[block_index_[current_fill]];
        GetFromBlockId(&cached_blocks_[current_fill].data,
                       cached_blocks_[current_fill].block_addr);
      }
    }
  }

  void RecursiveIndexIncrease(int current_depth) {
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

  const Ext2Inode& inode_;
  size_t current_pos = 0;

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

  // Block that only contains the block addresse. Used by doubly and trebly
  // address blocks.
  using AddressBlock = std::array<uint32_t, kMaxBlockEntryInBlock>;

  struct CachedBlock {
    uint32_t block_addr = -1;
    AddressBlock data;
  };

  std::array<CachedBlock, 4> cached_blocks_;
};

}  // namespace

Ext2FileSystem::Ext2FileSystem() {
  GetFromBlockId(&super_block_, 1);

  num_block_desc_ = integer_ratio_round_up(super_block_.blocks_count,
                                           super_block_.blocks_per_group);
  block_descs_ = (Ext2BlockGroupDescriptor*)kmalloc(
      sizeof(Ext2BlockGroupDescriptor) * num_block_desc_);
  GetArrayFromBlockId(block_descs_, num_block_desc_, 2);

  std::array<Ext2Inode, 2> inode_table;
  GetFromBlockId(&inode_table, block_descs_[0].inode_table);

  for (int i = 0; i < 2; i++) {
    PrintInodeInfo(inode_table[i]);
  }

  root_inode_ = inode_table[1];
  root_dir_ = reinterpret_cast<uint8_t*>(kmalloc(root_inode_.size));

  ReadFile(root_inode_, root_dir_, root_inode_.size);
  ParseDirectory(root_dir_, root_inode_.size);

  /*
  uint8_t data[1024];
  ReadFileFromStart(inode_table[1], data, 1024);
  */

  // PrintInodeInfo(root_inode_);
  /*
  uint8_t data2[2048 * 10];
  */
  auto node = ReadInode(0xe);
  PrintInodeInfo(node);
}

Ext2Inode Ext2FileSystem::ReadInode(size_t inode_addr) {
  // Block group that the inode belongs to.
  size_t block_group_index = (inode_addr - 1) / super_block_.inodes_per_group;

  // Index of the inode within the block group.
  size_t index = (inode_addr - 1) % super_block_.inodes_per_group;

  size_t inode_table_block_id = block_descs_[block_group_index].inode_table;

  // Single "Block" contains (block_size / inode_size = 8) inodes.
  size_t block_containing_inode = inode_table_block_id + index / 8;

  kprintf("Size of %x / Block size : %x \n", sizeof(Ext2Inode), kBlockSize);
  kprintf(
      "Block group index : (%x) index : (%x) inode table block id : (%x), "
      "block_contain_inode : (%d) \n",
      block_group_index, index, inode_table_block_id, block_containing_inode);

  std::array<Ext2Inode, 8> block_with_inodes;
  GetFromBlockId(&block_with_inodes, block_containing_inode);
  return block_with_inodes[index % 8];
}

void Ext2FileSystem::ReadFile(const Ext2Inode& file_inode, uint8_t* buf,
                              size_t num_read, size_t offset) {
  BlockIterator iter(file_inode);
  iter.SetOffset(offset);

  size_t read = 0;
  for (size_t cur = offset; cur < offset + num_read; cur += kBlockSize) {
    kprintf("cur : %d num read : %d read ; %d \n", cur, num_read, read);
    Block block = *iter;
    size_t current_block_offset = 0;
    if (iter.Pos() < offset) {
      current_block_offset = (offset - iter.Pos());
    }
    for (; read < num_read && current_block_offset < kBlockSize;
         read++, current_block_offset++) {
      buf[read] = block[current_block_offset];
    }

    ++iter;
  }
}

}  // namespace Kernel
