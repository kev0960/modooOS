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

    [[maybe_unused]] void ReadFileFromStart(const Ext2Inode& inode,
                                            uint8_t* buf, int num_read) {
  Block block;
  size_t current = 0;

  // First iterate through direct blocks.
  for (int i = 0; i < 12; i++) {
    kprintf("Reading block at : %x \n", inode.block[i]);
    GetFromBlockId(&block, inode.block[i]);
    memcpy(static_cast<void*>(buf + current), &block, min(num_read, 1024));
    num_read -= kBlockSize;

    if (num_read <= 0) {
      break;
    }
  }
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
  BlockIterator() : current_depth_(0) { block_index_.fill(0); }

  BlockIterator& operator++() {
    RecursiveIndexIncrease(current_depth_);
    return *this;
  }

  std::array<uint32_t, 4> GetBlockIndex() const { return block_index_; }

 private:
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

  // 0 : direct block.
  // 1 : single indirect block.
  // 2 : doubly indirect block.
  // 3 : triply indirect block.
  int current_depth_;

  // Index of each block at each level. E.g if this block is double indirect
  // block, then it can be {13, 4, 5}
  //
  // File Inode: block[13] = 10.
  // At block 10, we are at block[4] = 30.
  // At block 30, we are at block[5] = 50.
  std::array<uint32_t, 4> block_index_;
};

void ReadFileBlockByIterator(const Ext2Inode& file_inode,
                             const BlockIterator& itr) {
  auto block_index = itr.GetBlockIndex();
}

}  // namespace

Ext2FileSystem::Ext2FileSystem() {
  GetFromBlockId(&super_block_, 1);

  /*
  kprintf("inodes count : %d \n", super_block_.inodes_count);
  kprintf("block count : %d \n", super_block_.blocks_count);
  kprintf("block size : %d \n", 1 << (10 + super_block_.log_block_size));
  kprintf("inodes per group : %d \n", super_block_.inodes_per_group);
  kprintf("blocks per group  : %d \n", super_block_.blocks_per_group);
  kprintf("magic : %x \n", super_block_.magic);
  if ((1 << (10 + super_block_.log_block_size)) != kBlockSize) {
    kprintf("Block size is not 1024 bytes :( \n");
  }*/
  num_block_desc_ = integer_ratio_round_up(super_block_.blocks_count,
                                           super_block_.blocks_per_group);
  block_descs_ = (Ext2BlockGroupDescriptor*)kmalloc(
      sizeof(Ext2BlockGroupDescriptor) * num_block_desc_);
  GetArrayFromBlockId(block_descs_, num_block_desc_, 2);

  /*
  for (size_t i = 0; i < 1; i++) {
    //    kprintf("block bitmap : %d \n", descriptor[i].block_bitmap);
    //    kprintf("inode bitmap : %d \n", descriptor[i].inode_bitmap);
    kprintf("inode table : %d \n", descriptor[i].inode_table);
    kprintf("num dirs : %d \n", descriptor[i].used_dirs_count);
    kprintf("free blocks cnt : %d \n", descriptor.free_blocks_count);
    kprintf("free inode cnt : %d \n", descriptor.free_inodes_count);
    kprintf("num dirs : %d \n", descriptor.used_dirs_count);
  }

  auto inodes_per_block = kBlockSize / sizeof(Ext2Inode);
  kprintf("Inodes per block : %d \n", kBlockSize / sizeof(Ext2Inode));
  kprintf("Num Inode table blocks : %d\n",
          super_block_.inodes_per_group / inodes_per_block);
          */
  // Block block;
  Ext2Inode* inode_table = (Ext2Inode*)kmalloc(sizeof(Ext2Inode) * 8);
  GetArrayFromBlockId(inode_table, 8, block_descs_[0].inode_table);

  for (int i = 0; i < 8; i++) {
    PrintInodeInfo(inode_table[i]);
  }

  uint8_t data[1024];
  ReadFileFromStart(inode_table[1], data, 1024);
  ParseDirectory(data, 1024);

  /*
  kprintf("Inode size: %d \n", super_block_.inode_size);
  kprintf("Total block groups : %d \n",
          integer_ratio_round_up(super_block_.blocks_count,
                                 super_block_.blocks_per_group));
  kprintf("blocks per group : %d \n", super_block_.blocks_per_group);
  kprintf("total block : %d \n", super_block_.blocks_count);
  kprintf("inodes per group : %d \n", super_block_.inodes_per_group);

  PrintInodeInfo(ReadInode(0xc));
  kprintf("inode table : %d \n", block_descs_[0].inode_table);
  kprintf("num inodes per block : %d \n", kBlockSize / super_block_.inode_size);
  */
  // PrintInodeInfo(ReadInode(0x2));

  uint8_t data2[2048];
  auto node = ReadInode(0xc);
  PrintInodeInfo(node);
  ReadFileFromStart(node, data2, node.size);
  for (size_t i = 0; i < node.size; i++) {
    kprintf("%c", data2[i]);
  }
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
  /*
  for (int i = 0; i < 8; i++) {
    PrintInodeInfo(block_with_inodes[i]);
  }
*/
  return block_with_inodes[index % 8];
}

void Ext2FileSystem::ReadFile(const Ext2Inode& file_inode, uint8_t* buf,
                              size_t num_read, size_t offset) {}

}  // namespace Kernel
