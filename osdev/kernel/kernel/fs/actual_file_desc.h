#ifndef FS_ACTUAL_FILE_DESC_h
#define FS_ACTUAL_FILE_DESC_h

#include "../../std/types.h"
#include "../file_descriptor.h"
#include "ext2.h"

namespace Kernel {

// File descriptor that points to the real file.
class ActualFileDescriptor : public FileDescriptor {
 public:
  ActualFileDescriptor(int inode_num)
      : inode_num_(inode_num), offset_(0), inode_(nullptr) {}

  int GetInodeNum() const { return inode_num_; }
  DescriptorType GetDescriptorType() final { return ACTUAL_FILE; }

  size_t Read(void* buf, int count);
  size_t Write(void* buf, int count);

  enum Whence { SEEK_SET, SEEK_CUR, SEEK_END };
  off_t Seek(off_t offset, Whence whence);

  FileInfo Stat();

 private:
  int inode_num_;
  size_t offset_;

  Ext2Inode* inode_;
};

};  // namespace Kernel

#endif
