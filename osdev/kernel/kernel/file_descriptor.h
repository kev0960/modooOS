#ifndef FILE_DESCRIPTOR_H
#define FILE_DESCRIPTOR_H

#include "../std/map.h"
#include "qemu_log.h"

namespace Kernel {

class FileDescriptor {
 public:
  enum DescriptorType { UNKNOWN, ACTUAL_FILE, PIPE_READ, PIPE_WRITE };

  virtual DescriptorType GetDescriptorType() { return UNKNOWN; }

  // TODO Add locks here.
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

  virtual ~FileDescriptor() = default;

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

  FileDescriptorTable(const FileDescriptorTable& table) {
    fd_to_desc_ = table.fd_to_desc_;
  }

  FileDescriptorTable& operator=(const FileDescriptorTable& table) {
    QemuSerialLog::Logf("Copy table!");
    fd_to_desc_ = table.fd_to_desc_;
    return *this;
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

  // Returns the previous descriptor if fd_to_desc[fd] already exists.
  FileDescriptor* SetDescriptor(int fd, FileDescriptor* desc) {
    auto itr = fd_to_desc_.find(fd);
    if (itr == fd_to_desc_.end()) {
      fd_to_desc_[fd] = desc;
      return nullptr;
    } else {
      FileDescriptor* prev = (*itr).second;
      fd_to_desc_[fd] = desc;
      return prev;
    }
  }

  void AddProcessIdToDescriptors(pid_t pid) {
    for (auto itr = fd_to_desc_.begin(); itr != fd_to_desc_.end(); ++itr) {
      if ((*itr).second != nullptr) {
        (*itr).second->AddProcess(pid);
      }
    }
  }

  void RemoveProcessIdToDescriptors(pid_t pid) {
    for (auto itr = fd_to_desc_.begin(); itr != fd_to_desc_.end(); ++itr) {
      if ((*itr).second != nullptr) {
        (*itr).second->RemoveProcess(pid);
      }
    }
    // TODO Remove descriptors that are not being used.
  }

 //private:
  std::map<int, FileDescriptor*> fd_to_desc_;
};

}  // namespace Kernel

#endif
