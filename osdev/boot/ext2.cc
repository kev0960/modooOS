#include "ext2.h"
#include "array.h"
#include "ata.h"
#include "printf.h"

namespace Kernel {
constexpr size_t kBlockSize = 1024;

using Block = std::array<uint8_t, kBlockSize>;

namespace {

template <typename T>
void GetBlockFromBlockId(T* t, size_t block_id) {
  // sector size is 512 bytes. That means, 1 block spans 2 sectors.
  ATADriver::GetATADriver().Read(t, 2 * block_id);
}

}  // namespace

Ext2FileSystem::Ext2FileSystem() {
  auto& ata = ATADriver::GetATADriver();

  ata.Read(&super_block_, 2);
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
  }

  Ext2BlockGroupDescriptor descriptor[12];
  ata.Read((uint8_t*)descriptor, sizeof(descriptor), 4);
  // GetBlockFromBlockId(&descriptor, 2);

  for (size_t i = 0; i < 1; i++) {
    //    kprintf("block bitmap : %d \n", descriptor[i].block_bitmap);
    //    kprintf("inode bitmap : %d \n", descriptor[i].inode_bitmap);
    kprintf("inode table : %d \n", descriptor[i].inode_table);
    kprintf("num dirs : %d \n", descriptor[i].used_dirs_count);
    /*
    kprintf("free blocks cnt : %d \n", descriptor.free_blocks_count);
    kprintf("free inode cnt : %d \n", descriptor.free_inodes_count);
    kprintf("num dirs : %d \n", descriptor.used_dirs_count);*/
  }

  auto inodes_per_block = kBlockSize / sizeof(Ext2Inode);
  kprintf("Inodes per block : %d \n", kBlockSize / sizeof(Ext2Inode));
  kprintf("Num Inode table blocks : %d\n",
          super_block_.inodes_per_group / inodes_per_block);

  Block block;
  /*
  Ext2Inode inodes[4];
  ata.Read(reinterpret_cast<uint8_t*>(inodes), sizeof(inodes),
           2 * descriptor[0].inode_table);
  for (int i = 0; i < 4; i++) {
    kprintf("Inode %d : size : %x  : mode : %x %x %x %x \n", i, inodes[i].size,
            inodes[i].mode, inodes[i].atime, inodes[i].ctime);
    kprintf("dir ? %d \n", inodes[i].mode & 0x4000);
  }*/
  GetBlockFromBlockId(&block, descriptor[0].inode_table);
  for (size_t i = 0; i < kBlockSize / 2; i ++) {
    kprintf("%x ", block[i]);
  }
}
}  // namespace Kernel
