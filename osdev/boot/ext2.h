#ifndef EXT2_H
#define EXT2_H

#include "types.h"

namespace Kernel {
class Ext2FileSystem {
 public:
  Ext2FileSystem(const Ext2FileSystem&) = delete;
  Ext2FileSystem operator=(const Ext2FileSystem&) = delete;

  static Ext2FileSystem& GetExt2FileSystem() {
    static Ext2FileSystem ext2;
    return ext2;
  }

 private:
  Ext2FileSystem();
};
}  // namespace Kernel

#endif
