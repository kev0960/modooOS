#include "ext2.h"

#include "../../std/algorithm.h"
#include "../../std/array.h"
#include "../../std/printf.h"
#include "../../std/string.h"
#include "../apic.h"
#include "../cpu_context.h"
#include "../kernel_util.h"
#include "../kmalloc.h"
#include "../qemu_log.h"
#include "ata.h"
#include "block_iterator.h"

namespace Kernel {
namespace {

constexpr uint8_t kDirFileTypeRegularFile = 1;
constexpr uint8_t kDirFileTypeDir = 2;
constexpr uint16_t kInodeFileTypeRegularFile = 0x8000;
constexpr uint16_t kInodeFileTypeDir = 0x4000;
constexpr uint32_t kRootInodeNumber = 2;

void ParseInodeMode(uint16_t mode) {
  kprintf("Mode :: ");
  const char* file_formats[] = {
      "fifo", "character device", "dir", "block device", "file", "sym", "sock"};
  uint16_t file_format_modes[] = {0x1000, 0x2000, 0x4000, 0x6000,
                                  0x8000, 0xA000, 0xC000};

  for (int i = 0; i < 7; i++) {
    if ((mode & file_format_modes[i]) == file_format_modes[i]) {
      kprintf("%s ", file_formats[i]);
    }
  }

  const char* access[] = {"r", "w", "x", "r", "w", "x", "r", "w", "x"};
  uint16_t access_modes[] = {0x100, 0x80, 0x40, 0x20, 0x10, 0x8, 0x4, 0x2, 0x1};
  for (int i = 0; i < 9; i++) {
    if ((mode & access_modes[i]) == access_modes[i]) {
      kprintf("%s", access[i]);
    } else {
      kprintf("-");
    }
  }
  kprintf("\n");
}

bool IsDirectory(const Ext2Inode& inode) { return (inode.mode & 0x4000); }

[[maybe_unused]] void PrintInodeInfo(const Ext2Inode& inode) {
  kprintf("Inode Block Info ----------- [cpu : %d]\n",
          CPUContextManager::GetCPUContextManager().GetCPUContext()->cpu_id);
  kprintf("Size (%x) Created at (%x) Num blocks (%x) ", inode.size, inode.ctime,
          inode.blocks);
  ParseInodeMode(inode.mode);
  for (size_t i = 0; i < min((uint16_t)15, inode.blocks); i++) {
    if (inode.block[i]) {
      kprintf("[%x] ", inode.block[i]);
    }
  }
  kprintf("\n");
}

template <typename T>
T ReadAndAdvance(uint8_t*& buf) {
  T t = *(T*)(buf);
  buf += sizeof(T);
  return t;
}

template <typename T>
void SetAndAdvance(uint8_t*& buf, T data) {
  *(T*)(buf) = data;
  buf += sizeof(T);
}

[[maybe_unused]] void PrintDirectory(uint8_t* buf, int num_read) {
  int current = 0;
  while (current < num_read) {
    uint8_t* prev = buf;

    // Read inode number.
    uint32_t inode = ReadAndAdvance<uint32_t>(buf);

    // Read total entry size.
    uint16_t entry_size = ReadAndAdvance<uint16_t>(buf);

    if (entry_size == 0) break;

    // Read name length.
    uint8_t name_len = ReadAndAdvance<uint8_t>(buf);

    // Read type indicator.
    uint8_t file_type = ReadAndAdvance<uint8_t>(buf);

    kprintf("Inode : (%x) Entry size : (%x) Name Len : (%x) File type : (%x) ",
            inode, entry_size, name_len, file_type);
    for (size_t i = 0; i < name_len; i++) {
      kprintf("%c", buf[i]);
    }
    kprintf("\n");
    buf = prev + entry_size;
    current += entry_size;
  }
}

}  // namespace

Ext2FileSystem::Ext2FileSystem() {
  GetFromBlockId(&super_block_, 1);

  num_block_desc_ = integer_ratio_round_up(super_block_.blocks_count,
                                           super_block_.blocks_per_group);
  block_descs_ = (Ext2BlockGroupDescriptor*)kmalloc(
      sizeof(Ext2BlockGroupDescriptor) * num_block_desc_);
  GetArrayFromBlockId(block_descs_, num_block_desc_, 2);

  ASSERT((1024 << super_block_.log_block_size) == kBlockSize);
  kprintf("super block inode : %d \n", super_block_.inode_size);
  ASSERT(super_block_.inode_size == sizeof(Ext2Inode));

  std::array<Ext2Inode, 2> inode_table;
  GetFromBlockId(&inode_table, block_descs_[0].inode_table);

  root_inode_ = inode_table[1];
  root_dir_ = ParseDirectory(&root_inode_);
  block_bitmap.reserve(num_block_desc_);
  inode_bitmap.reserve(num_block_desc_);
  for (size_t i = 0; i < num_block_desc_; i++) {
    BitmapInfo bitmap;
    bitmap.bitmap_block_id = block_descs_[i].block_bitmap;
    GetArrayFromBlockId(reinterpret_cast<uint8_t*>(bitmap.bitmap.GetBitmap()),
                        1024, bitmap.bitmap_block_id);
    block_bitmap.push_back(bitmap);

    bitmap.bitmap_block_id = block_descs_[i].inode_bitmap;
    GetArrayFromBlockId(reinterpret_cast<uint8_t*>(bitmap.bitmap.GetBitmap()),
                        1024, bitmap.bitmap_block_id);
    inode_bitmap.push_back(bitmap);
  }

  /*
  int num_wrote = 0;
  for (int i = 0; i < 5001; i++) {
    char data[10];
    sprintf(data, "%d,", i + 10000);
    WriteFile("/a.txt", (uint8_t*)data, strlen(data), num_wrote);
    num_wrote += strlen(data);
  }
  */

  /*
  for (int i = 0; i < 5000; i++) {
    char data[10];
    sprintf(data, "%d,", i + 12345);
    WriteFile("/a.txt", (uint8_t*)data, strlen(data));

    char data2[10];
    ReadFile("/a.txt", (uint8_t*)data2, strlen(data));
    kprintf("%s ", data2);
  }
  */

  /*
  CreateFile("/zz.txt", false);
  char data[10] = {"hello"};
  WriteFile("/zz.txt", (uint8_t*)data, strlen(data));

  kprintf("Write done");
  auto info = Stat("/a.txt/");
  kprintf(" size : %d %d\n", info.file_size, info.inode);
  uint8_t* file = (uint8_t*)kmalloc(1000);
  ReadFile("/a.txt/", file, 1000, 0);
  CreateFile("/zz.txt", false);

  char data[10] = {"hello"};
  WriteFile("/zz.txt", (uint8_t*)data, strlen(data));

  auto info = Stat("/zz.txt/");
  kprintf(" size : %d %d\n", info.file_size, info.inode);
  */
}

size_t Ext2FileSystem::ReadFile(std::string_view path, uint8_t* buf,
                                size_t num_read, size_t offset) {
  int inode_num = GetInodeNumberFromPath(path);
  if (inode_num == -1) {
    kprintf("File is not found!\n");
    return 0;
  }

  Ext2Inode file = ReadInode(inode_num);
  if (offset >= file.size) {
    return 0;
  }

  // Prevent reading more than the file size.
  size_t num_actually_read = min(num_read, file.size - offset);
  ReadFile(&file, buf, num_actually_read, offset);

  return num_actually_read;
}

Ext2Inode Ext2FileSystem::ReadInode(size_t inode_addr) {
  // Block group that the inode belongs to.
  size_t block_group_index = (inode_addr - 1) / super_block_.inodes_per_group;

  // Index of the inode within the block group.
  size_t index = (inode_addr - 1) % super_block_.inodes_per_group;

  size_t inode_table_block_id = block_descs_[block_group_index].inode_table;

  // Single "Block" contains (block_size / inode_size = 8) inodes.
  size_t block_containing_inode = inode_table_block_id + index / 8;

  std::array<Ext2Inode, 8> block_with_inodes;
  GetFromBlockId(&block_with_inodes, block_containing_inode);
  return block_with_inodes[index % 8];
}

void Ext2FileSystem::WriteInode(size_t inode_addr, const Ext2Inode& inode) {
  // Block group that the inode belongs to.
  size_t block_group_index = (inode_addr - 1) / super_block_.inodes_per_group;

  // Index of the inode within the block group.
  size_t index = (inode_addr - 1) % super_block_.inodes_per_group;

  size_t inode_table_block_id = block_descs_[block_group_index].inode_table;

  // Single "Block" contains (block_size / inode_size = 8) inodes.
  size_t block_containing_inode = inode_table_block_id + index / 8;

  std::array<Ext2Inode, 8> block_with_inodes;
  GetFromBlockId(&block_with_inodes, block_containing_inode);

  block_with_inodes[index % 8] = inode;
  WriteFromBlockId(block_with_inodes.data(), block_containing_inode);
}

size_t Ext2FileSystem::ReadFile(Ext2Inode* file_inode, uint8_t* buf,
                                size_t num_read, size_t offset) {
  BlockIterator iter(file_inode);
  iter.SetOffset(offset);

  size_t start_block_index = offset / kBlockSize;
  size_t end_block_index;

  if ((offset + num_read) % kBlockSize == 0) {
    end_block_index = (offset + num_read) / kBlockSize;
  } else {
    end_block_index = (offset + num_read) / kBlockSize + 1;
  }

  size_t read = 0;
  for (; start_block_index < end_block_index; start_block_index++) {
    Block block = *iter;
    size_t current_block_offset = 0;
    if (iter.Pos() < offset) {
      current_block_offset = (offset - iter.Pos());
    }
    for (; read < num_read && current_block_offset < kBlockSize;
         read++, current_block_offset++) {
      buf[read] = block[current_block_offset];
    }

    ++iter;
  }

  return read;
}

void Ext2FileSystem::WriteFile(std::string_view path, uint8_t* buf,
                               size_t num_write, size_t offset) {
  int inode_num = GetInodeNumberFromPath(path);
  kprintf("Write file : %d %s\n", inode_num, path);
  if (inode_num == -1) {
    return;
  }

  WriteFile(inode_num, buf, num_write, offset);
}

void Ext2FileSystem::WriteFile(size_t inode_num, uint8_t* buf, size_t num_write,
                               size_t offset) {
  Ext2Inode file_inode = ReadInode(inode_num);
  kprintf("Write: [%d] num : %d off : %d Size : %d \n", inode_num, num_write,
          offset, file_inode.size);

  // Expand the block first.
  if (offset + num_write >= file_inode.size) {
    kprintf("Expanding %d --> %d \n", file_inode.size, offset + num_write);
    ExpandFileSize(inode_num, offset + num_write);
    file_inode = ReadInode(inode_num);
    kprintf("New file isze : %d \n", file_inode.size);
  }

  BlockIterator iter(&file_inode);
  iter.SetOffset(offset);

  size_t start_block_index = offset / kBlockSize;
  size_t end_block_index;

  if ((offset + num_write) % kBlockSize == 0) {
    end_block_index = (offset + num_write) / kBlockSize;
  } else {
    end_block_index = (offset + num_write) / kBlockSize + 1;
  }

  size_t write = 0;
  for (; start_block_index < end_block_index; start_block_index++) {
    size_t current_block_offset = 0;
    if (iter.Pos() < offset) {
      current_block_offset = (offset - iter.Pos());
    }
    // TODO You don't have to read the block if you are writing to the entire
    // block.
    Block block = *iter;
    for (; write < num_write && current_block_offset < kBlockSize;
         write++, current_block_offset++) {
      block[current_block_offset] = buf[write];
    }

    WriteFromBlockId(block.data(), iter.GetDataBlockID());
    ++iter;
  }
}

void Ext2FileSystem::ExpandFileSize(size_t inode_num,
                                    size_t expanded_file_size) {
  Ext2Inode file_inode = ReadInode(inode_num);
  size_t current_file_size = file_inode.size;
  if (current_file_size == 0) {
    kprintf("Create new file!");
    file_inode.size = min(kBlockSize, expanded_file_size);
    current_file_size = file_inode.size;

    file_inode.block[0] = GetEmptyBlock();
    MarkEmptyBlockAsUsed(file_inode.block[0]);
    WriteInode(inode_num, file_inode);
  }

  // ASSERT(current_file_size != 0);

  kprintf("Size : %d --> %d \n", current_file_size, expanded_file_size);
  if (current_file_size >= expanded_file_size) {
    return;
  }

  file_inode.size = expanded_file_size;
  WriteInode(inode_num, file_inode);

  auto prev = BlockIterator(&file_inode);
  prev.SetOffset(current_file_size - 1);
  auto end = BlockIterator(&file_inode);
  end.SetOffset(expanded_file_size - 1);

  auto curr = prev;

  if (curr == end) {
    return;
  }

  // TODO Batch the writes to the device.
  while (true) {
    for (size_t i = 0; i <= curr.Index().CurrentDepth(); i++) {
      if (i > prev.Index().CurrentDepth() ||
          prev.Index()[i] != curr.Index()[i]) {
        size_t empty_block_id = GetEmptyBlock();
        MarkEmptyBlockAsUsed(empty_block_id);
        file_inode.blocks++;
        // curr.SetBlockId(i, empty_block_id);

        kprintf("Diff! Empty block : [%d] at %d\n", empty_block_id, i);
        if (i == 0) {
          file_inode.block[curr.Index()[0]] = empty_block_id;
          // Need to update the inode.
          WriteInode(inode_num, file_inode);
        } else {
          Block* block = curr.GetBlockFromDepth(i - 1);
          BlockIterator::SetNthEntryAtAddressBlock(block, curr.Index()[i],
                                                   empty_block_id);
          WriteFromBlockId(block->data(), curr.GetBlockId(i - 1));
          kfree(block);
        }
      }
    }
    if (curr == end) {
      break;
    }

    prev = curr;
    ++curr;
  }
}

size_t Ext2FileSystem::GetEmptyBlock() {
  std::lock_guard<MultiCoreSpinLock> lk(fs_lock_);

  for (size_t i = 0; i < block_bitmap.size(); i++) {
    BitmapInfo& block_info = block_bitmap[i];
    int index_in_group = block_info.bitmap.GetEmptyBitIndex();
    if (index_in_group != -1) {
      kprintf("index : (%d) %lx ", i, index_in_group);
      return index_in_group + super_block_.blocks_per_group * i;
    }
  }
  return 0;
}

void Ext2FileSystem::MarkEmptyBlockAsUsed(size_t block_id) {
  size_t block_group_index = block_id / super_block_.blocks_per_group;
  BitmapInfo& block_info = block_bitmap[block_group_index];
  block_info.bitmap.FlipBit(block_id % super_block_.blocks_per_group);

  WriteFromBlockId(block_info.bitmap.GetBitmap(), block_info.bitmap_block_id);
}

size_t Ext2FileSystem::GetEmptyInode() {
  std::lock_guard<MultiCoreSpinLock> lk(fs_lock_);

  for (size_t i = 0; i < inode_bitmap.size(); i++) {
    BitmapInfo& inode_info = inode_bitmap[i];
    int index_in_group = inode_info.bitmap.GetEmptyBitIndex();
    if (index_in_group != -1) {
      kprintf("index : (%d) %lx ", i, index_in_group);
      return index_in_group + super_block_.inodes_per_group * i + 1;
    }
  }
  return 0;
}

void Ext2FileSystem::MarkEmptyInodeAsUsed(size_t inode_num) {
  inode_num--;
  size_t inode_group_index = inode_num / super_block_.inodes_per_group;
  BitmapInfo& inode_info = inode_bitmap[inode_group_index];
  inode_info.bitmap.FlipBit(inode_num % super_block_.inodes_per_group);

  WriteFromBlockId(inode_info.bitmap.GetBitmap(), inode_info.bitmap_block_id);
}

FileInfo Ext2FileSystem::Stat(std::string_view path) {
  int inode_num = GetInodeNumberFromPath(path);
  if (inode_num == -1) {
    return FileInfo{};
  }

  Ext2Inode file = ReadInode(inode_num);
  // PrintInodeInfo(file);
  FileInfo info;
  info.file_size = file.size;
  info.inode = inode_num;
  info.mode = file.mode;

  return info;
}

bool Ext2FileSystem::CreateFile(std::string_view path, bool is_directory) {
  if (GetInodeNumberFromPath(path) != -1) {
    kprintf("File already exists!");
    return false;
  }

  size_t parent = path.find_last_of('/');
  if (parent == npos) {
    return false;
  }

  std::string_view parent_path = path.substr(0, parent + 1);
  int parent_inode_num = GetInodeNumberFromPath(parent_path);
  if (parent_inode_num == -1) {
    kprintf("Parent does not exist. [%s]", KernelString(parent_path).c_str());
    return false;
  }
  Ext2Inode parent_inode = ReadInode(parent_inode_num);
  if (!IsDirectory(parent_inode)) {
    kprintf("Parent is not a directory.");
    return false;
  }

  std::string_view file_name = path.substr(parent + 1);

  // 4 (inode) + 2 (entry_size) + 1 (file_type) + 1 (name_len) = 8
  uint16_t dir_entry_size = 8 + file_name.size();
  if (dir_entry_size % 4 != 0) {
    dir_entry_size = ((dir_entry_size / 4) + 1) * 4;
  }
  uint8_t* dir_data = reinterpret_cast<uint8_t*>(kmalloc(dir_entry_size));
  uint8_t* saved_dir_data = dir_data;

  kprintf("dir data : %lx \n", dir_data);
  // Now find an inode for the new file.
  uint32_t new_file_inode = GetEmptyInode();
  MarkEmptyInodeAsUsed(new_file_inode);

  SetAndAdvance(dir_data, new_file_inode);
  SetAndAdvance(dir_data, dir_entry_size);
  SetAndAdvance(dir_data, (uint8_t)(file_name.size()));

  kprintf("New file indoe : %d %d %d", new_file_inode, dir_entry_size,
          file_name.size());
  if (is_directory) {
    SetAndAdvance(dir_data, kDirFileTypeDir);
  } else {
    SetAndAdvance(dir_data, kDirFileTypeRegularFile);
  }

  // Now copy the filename.
  for (size_t i = 0; i < file_name.size(); i++) {
    dir_data[i] = file_name[i];
  }

  WriteFile(parent_inode_num, saved_dir_data, dir_entry_size,
            GetEndOfDirectoryEntry(&parent_inode));

  Ext2Inode new_inode = ReadInode(new_file_inode);
  new_inode.size = 0;
  if (is_directory) {
    new_inode.mode = kInodeFileTypeDir;
  } else {
    new_inode.mode = kInodeFileTypeRegularFile;
  }

  WriteInode(new_file_inode, new_inode);
  if (parent_inode_num == kRootInodeNumber) {
    Ext2Inode root_inode = ReadInode(kRootInodeNumber);
    root_dir_ = ParseDirectory(&root_inode);
  }
  return true;
}

std::vector<Ext2Directory> Ext2FileSystem::ParseDirectory(Ext2Inode* dir) {
  // Read the entire directory.
  uint8_t* dir_data = reinterpret_cast<uint8_t*>(kmalloc(dir->size));
  ReadFile(dir, dir_data, dir->size);

  std::vector<Ext2Directory> dir_info;

  size_t current = 0;
  uint8_t* current_read = dir_data;

  while (current < dir->size) {
    uint8_t* dir_start = current_read;

    // Read inode number.
    uint32_t inode = ReadAndAdvance<uint32_t>(current_read);

    // Read total entry size.
    uint16_t entry_size = ReadAndAdvance<uint16_t>(current_read);

    if (entry_size == 0) {
      break;
    }

    // Read name length.
    uint8_t name_len = ReadAndAdvance<uint8_t>(current_read);

    // Read type indicator.
    uint8_t file_type = ReadAndAdvance<uint8_t>(current_read);

    KernelString file_name(reinterpret_cast<char*>(current_read), name_len);
    Ext2Directory dir_entry;
    dir_entry.inode = inode;
    dir_entry.file_type = file_type;
    dir_entry.name = file_name;
    /*
    kprintf("Dir : %s inode (%lx) (%lx) (%lx) entry size(%lx)\n",
            file_name.c_str(), inode, file_type, name_len, entry_size);
    */
    dir_info.push_back(dir_entry);

    current_read = dir_start + entry_size;
    current += entry_size;
  }

  return dir_info;
}

size_t Ext2FileSystem::GetEndOfDirectoryEntry(Ext2Inode* dir_inode) {
  uint8_t* dir_data = reinterpret_cast<uint8_t*>(kmalloc(dir_inode->size));
  ReadFile(dir_inode, dir_data, dir_inode->size);

  size_t current = 0;
  uint8_t* current_read = dir_data;

  while (current < dir_inode->size) {
    uint8_t* dir_start = current_read;

    // Read inode number.
    ReadAndAdvance<uint32_t>(current_read);

    // Read total entry size.
    uint16_t entry_size = ReadAndAdvance<uint16_t>(current_read);
    // kprintf("Etnry size ; %d \n", entry_size);
    if (entry_size == 0) {
      current_read -= (sizeof(uint32_t) + sizeof(uint16_t));
      return current_read - dir_data;
    }

    // Read name length.
    ReadAndAdvance<uint8_t>(current_read);

    // Read type indicator.
    ReadAndAdvance<uint8_t>(current_read);

    current_read = dir_start + entry_size;
    current += entry_size;
  }
  return current_read - dir_data;
}

int Ext2FileSystem::GetInodeNumberFromPath(std::string_view path) {
  if (path.empty() || path[0] != '/') {
    return -1;
  }

  // TODO Support relative paths.
  ASSERT(path[0] == '/');

  // Root inode number is 2.
  if (path.size() == 1) {
    return 2;
  }

  size_t current = 1;
  std::vector<Ext2Directory> current_dir = root_dir_;

  while (true) {
    size_t end = path.find_first_of('/', current);
    if (end != npos) {
      std::string_view name = path.substr(current, end - current);

      bool found = false;
      for (const auto& file : current_dir) {
        if (file.name == name) {
          // Case for /.../.../name/
          if (end == path.size() - 1) {
            return file.inode;
          } else {
            // Case for /.../name/...
            Ext2Inode inode = ReadInode(file.inode);
            current_dir = ParseDirectory(&inode);
            found = true;
            break;
          }
        }
      }
      if (!found) {
        return -1;
      }
      current = end + 1;
    } else {
      // Case for /.../.../name
      std::string_view name = path.substr(current);
      for (const auto& file : current_dir) {
        if (file.name == name) {
          return file.inode;
        }
      }
      return -1;
    }
  }
  return -1;
}

KernelString Ext2FileSystem::GetAbsolutePath(const KernelString& path,
                                             const KernelString& current_dir) {
  QemuSerialLog::Logf("Path : %s %d\n", path.c_str(), path.size());
  // current_dir must be an absolute dir.
  if (!(current_dir.size() > 0 && current_dir.at(0) == '/')) {
    return "";
  }

  if (path.size() == 0) {
    return current_dir;
  }

  // If path is already an absolute path.
  if (path.at(0) == '/') {
    return path;
  }

  KernelString p = current_dir;
  QemuSerialLog::Logf("p : %s %c %d\n", p.c_str(), p.back(), p.size());
  if (p.back() != '/') {
    p.append("/");
  }
  p.append(path);

  return p;
}

}  // namespace Kernel
