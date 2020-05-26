#ifndef FILE_DESCRIPTOR_H
#define FILE_DESCRIPTOR_H

#include "../std/map.h"

namespace Kernel {

class FileDescriptor {
 public:
  enum DescriptorType { UNKNOWN, ACTUAL_FILE, PIPE };

  virtual DescriptorType GetDescriptorType() { return UNKNOWN; }

  void AddProcess(pid_t pid) {
    auto itr = processes_.find(pid);
    if (itr == processes_.end()) {
      processes_.insert(pid);
    }
  }

  void RemoveProcess(pid_t pid) {
    auto itr = processes_.find(pid);
    if (itr != processes_.end()) {
      processes_.erase(itr);
    }
  }

  bool IsNotInUse() const { return processes_.size() == 0; }

 private:
  // List of proceses that has opened this descriptor.
  std::set<pid_t> processes_;
};

class FileDescriptorTable {
 public:
  enum STDIO { STDIN, STDOUT, STDERR };

  FileDescriptorTable() {
    fd_to_desc_[STDIN] = nullptr;
    fd_to_desc_[STDOUT] = nullptr;
    fd_to_desc_[STDERR] = nullptr;
  }

  FileDescriptor* GetDescriptor(int fd) {
    auto itr = fd_to_desc_.find(fd);
    if (itr == fd_to_desc_.end()) {
      return nullptr;
    }
    return (*itr).second;
  }

  int AddDescriptor(FileDescriptor* desc) {
    int fd = fd_to_desc_.size();
    fd_to_desc_[fd] = desc;
    return fd;
  }

 private:
  std::map<int, FileDescriptor*> fd_to_desc_;
};

}  // namespace Kernel

#endif
