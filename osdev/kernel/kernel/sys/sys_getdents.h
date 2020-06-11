#ifndef SYS_SYS_GETDENTS_H
#define SYS_SYS_GETDENTS_H

#include "../fs/actual_file_desc.h"
#include "../fs/ext2.h"
#include "../process.h"
#include "sys.h"

namespace Kernel {
namespace {

template <typename T>
char* WriteBufferAndAdvance(const T& val, char* buffer) {
  *reinterpret_cast<T*>(buffer) = val;
  return buffer + sizeof(T);
}

}  // namespace

class SysGetDentsHandler : public SyscallHandler<SysGetDentsHandler> {
 public:
  int SysGetDents(int fd, struct linux_dirent* dirp, size_t count) {
    ASSERT(!KernelThread::CurrentThread()->IsKernelThread());
    Process* process = static_cast<Process*>(KernelThread::CurrentThread());

    FileDescriptorTable& table = process->GetFileDescriptorTable();
    FileDescriptor* desc = table.GetDescriptor(fd);

    if (desc->GetDescriptorType() != FileDescriptor::ACTUAL_FILE) {
      return 0;
    }

    ActualFileDescriptor* actual_file_desc =
        static_cast<ActualFileDescriptor*>(desc);
    FileInfo info = actual_file_desc->Stat();

    // Return if the file is not a directory.
    if (Ext2FileSystem::GetFileFormatFromMode(info.mode) !=
        Ext2FileSystem::S_DIR) {
      return 0;
    }

    return actual_file_desc->Read(reinterpret_cast<char*>(dirp), count);

    /*
    auto& ext2 = Ext2FileSystem::GetExt2FileSystem();
    Ext2Inode inode = ext2.ReadInode(actual_file_desc->GetInodeNum());
    std::vector<Ext2Directory> dirs = ext2.ParseDirectory(&inode);

    static constexpr int kLinuxDirentSizeWithoutName =
        sizeof(uint32_t) * 2 + sizeof(uint16_t);

    char* buffer = reinterpret_cast<char*>(dirp);
    int buffer_index = 0;
    for (const auto& dir : dirs) {
      // If the buffer has enough space, then we start copying :)
      if (count - buffer_index >=
          kLinuxDirentSizeWithoutName + dir.name.size()) {
        buffer = WriteBufferAndAdvance(dir.inode, buffer);
        buffer = WriteBufferAndAdvance(
            kLinuxDirentSizeWithoutName + dir.name.size(), buffer);
        buffer = WriteBufferAndAdvance(
            kLinuxDirentSizeWithoutName + dir.name.size(), buffer);
        for (size_t i = 0; i < dir.name.size(); i++) {
          buffer = WriteBufferAndAdvance(dir.name.at(i), buffer);
        }
      } else {
        break;
      }
    }*/
  }
};
}  // namespace Kernel

#endif
