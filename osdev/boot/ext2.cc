#include "ext2.h"
#include "algorithm.h"
#include "array.h"
#include "ata.h"
#include "printf.h"
#include "string.h"

namespace Kernel {
constexpr size_t kBlockSize = 1024;

using Block = std::array<uint8_t, kBlockSize>;

namespace {

template <typename T>
void GetBlockFromBlockId(T* t, size_t block_id) {
  // sector size is 512 bytes. That means, 1 block spans 2 sectors.
  ATADriver::GetATADriver().Read(t, 2 * block_id);
}

void PrintInodeInfo(const Ext2Inode& inode) {
  kprintf("Inode Block Info ----------- \n");
  kprintf("Mode (%08b) Size (%x) Created at (%x) Num blocks (%x) \n",
          inode.mode, inode.size, inode.ctime, inode.blocks);
  for (size_t i = 0; i < min((uint16_t)15, inode.blocks); i++) {
    if (inode.block[i]) {
      kprintf("[%x] ", inode.block[i]);
    }
  }
  kprintf("\n");
}

void ReadFileFromStart(const Ext2Inode& inode, uint8_t* buf, int num_read) {
  Block block;
  size_t current = 0;

  // First iterate through direct blocks.
  for (int i = 0; i < 12; i++) {
    kprintf("Reading block at : %x \n", inode.block[i]);
    GetBlockFromBlockId(&block, inode.block[i]);
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

void ParseDirectory(uint8_t* buf, int num_read) {
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

    kprintf(
        "Inode : (%x) Entry size : (%x) Name Len : (%x) File type : (%x) ",
        inode, entry_size, name_len, file_type);
    for (size_t i = 0; i < name_len; i ++) {
      kprintf("%c", buf[i]);
    }
    kprintf("\n");
    buf = prev + entry_size;
    current += entry_size;
  }
}

}  // namespace

Ext2FileSystem::Ext2FileSystem() {
  auto& ata = ATADriver::GetATADriver();

  ata.Read(&super_block_, 2);
  /*
  kprintf("inodes count : %d \n", super_block_.inodes_count);
  kprintf("block count : %d \n", super_block_.blocks_count);
  kprintf("block size : %d \n", 1 << (10 + super_block_.log_block_size));
  kprintf("inodes per group : %d \n", super_block_.inodes_per_group);
  kprintf("blocks per group  : %d \n", super_block_.blocks_per_group);
  kprintf("Total block groups : %d \n",
          super_block_.blocks_count / super_block_.blocks_per_group);
  kprintf("magic : %x \n", super_block_.magic);
  if ((1 << (10 + super_block_.log_block_size)) != kBlockSize) {
    kprintf("Block size is not 1024 bytes :( \n");
  }*/

  kprintf("??\n");
  Ext2BlockGroupDescriptor descriptor[12];
  GetBlockFromBlockId(&descriptor, 2);

  kprintf("block bitmap : %d \n", descriptor[0].block_bitmap);
  kprintf("inode bitmap : %d \n", descriptor[0].inode_bitmap);
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
  std::array<Ext2Inode, 8> inode_table;
  GetBlockFromBlockId(&inode_table, descriptor[0].inode_table);

  /*
  ata.Read(reinterpret_cast<uint8_t*>(inodes), sizeof(inodes),
           2 * descriptor[0].inode_table);*/

  for (int i = 0; i < 8; i++) {
    PrintInodeInfo(inode_table[i]);
  }

  uint8_t data[2048];
  ReadFileFromStart(inode_table[1], data, 2048);
  ParseDirectory(data, 2048);

  /*GetBlockFromBlockId(&block, 1);
  for (size_t i = 0; i < kBlockSize / 2; i ++) {
    //kprintf("%x ", block[i]);
  }*/
  // ata.Read((uint8_t*)descriptor, sizeof(descriptor), 4);
}
}  // namespace Kernel
