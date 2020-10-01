#ifndef FS_ACTUAL_FILE_DESC_h
#define FS_ACTUAL_FILE_DESC_h

#include "../../std/types.h"
#include "../file_descriptor.h"
#include "ext2.h"

namespace Kernel {

// File descriptor that points to the real file.
class ActualFileDescriptor : public FileDescriptor {
 public:
  static constexpr int O_APPEND = (1 << 0);
  static constexpr int O_CREAT = (1 << 1);
  static constexpr int O_DIRECTORY = (1 << 2);
  static constexpr int O_TRUNC = (1 << 3);
  static constexpr int O_RDONLY = (1 << 4);
  static constexpr int O_WRONLY = (1 << 5);

  ActualFileDescriptor(int inode_num, int modes);

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
  int open_modes_;

  Ext2Inode* inode_;
};  // namespace Kernel
};  // namespace Kernel

#endif
