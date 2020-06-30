#include "actual_file_desc.h"

#include "ext2.h"

namespace Kernel {

size_t ActualFileDescriptor::Read(void* buf, int count) {
  QemuSerialLog::Logf("Read file : %d \n", inode_num_);
  auto& ext2 = Ext2FileSystem::GetExt2FileSystem();
  if (inode_ == nullptr) {
    inode_ = new Ext2Inode();
    *inode_ = ext2.ReadInode(inode_num_);
  }

  QemuSerialLog::Logf("Read inode : %d \n", inode_num_);
  size_t num_read =
      ext2.ReadFile(inode_, reinterpret_cast<uint8_t*>(buf), count, offset_);
  offset_ += num_read;

  return num_read;
}

size_t ActualFileDescriptor::Write(void* buf, int count) {
  auto& ext2 = Ext2FileSystem::GetExt2FileSystem();
  ext2.WriteFile(inode_num_, reinterpret_cast<uint8_t*>(buf), count, offset_);

  offset_ += count;
  return count;
}

off_t ActualFileDescriptor::Seek(off_t offset, Whence whence) {
  if (whence == SEEK_SET) {
    if (offset >= 0) {
      offset_ = offset;
    } else {
      return -1;
    }
  } else if (whence == SEEK_CUR) {
    if ((off_t)offset_ + offset >= 0) {
      offset_ += offset;
    } else {
      return -1;
    }
  } else if (whence == SEEK_END) {
    FileInfo stat = Stat();
    if ((off_t)stat.file_size + offset >= 0) {
      offset_ = stat.file_size + offset;
    } else {
      return -1;
    }
  }

  return offset_;
}

FileInfo ActualFileDescriptor::Stat() {
  auto& ext2 = Ext2FileSystem::GetExt2FileSystem();
  return ext2.Stat(inode_num_);
}

}  // namespace Kernel
