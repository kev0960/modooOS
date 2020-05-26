#include "actual_file_desc.h"

#include "ext2.h"

namespace Kernel {

size_t ActualFileDescriptor::Read(void* buf, int count) {
  auto& ext2 = Ext2FileSystem::GetExt2FileSystem();
  if (inode_ == nullptr) {
    inode_ = new Ext2Inode();
    *inode_ = ext2.ReadInode(inode_num_);
  }

  size_t num_read =
      ext2.ReadFile(inode_, reinterpret_cast<uint8_t*>(buf), count, offset_);
  offset_ += num_read;

  return num_read;
}

}  // namespace Kernel
