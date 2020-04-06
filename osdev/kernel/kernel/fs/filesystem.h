#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include "../../std/types.h"

namespace Kernel {

class FileSystem {
 public:
};

class File {
 public:
  enum STDIO { STDIN, STDOUT, STDERR, NOT_STDIO };

  File(STDIO std) : inode_num_(0), std_(std) {}
  File(size_t inode_num) : inode_num_(inode_num), std_(NOT_STDIO) {}

  bool IsStandardIO() { return inode_num_ == 0; }
  STDIO GetStdIO() { return std_; }
  size_t GetInodeNum() { return inode_num_; }

 private:
  size_t inode_num_;
  STDIO std_;
};

};  // namespace Kernel

#endif
