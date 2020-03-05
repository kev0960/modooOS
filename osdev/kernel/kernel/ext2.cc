#include "ext2.h"
#include "../std/algorithm.h"
#include "../std/array.h"
#include "../std/printf.h"
#include "../std/string.h"
#include "ata.h"
#include "kernel_util.h"
#include "kmalloc.h"

namespace Kernel {
constexpr size_t kBlockSize = 1024;
constexpr size_t kMaxBlockEntryInBlock = kBlockSize / sizeof(uint32_t);

using Block = std::array<uint8_t, kBlockSize>;

namespace {

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

template <typename T>
void GetFromBlockId(T* t, size_t block_id) {
  // sector size is 512 bytes. That means, 1 block spans 2 sectors.
  ATADriver::GetATADriver().Read(t, 2 * block_id);
}

template <typename T>
void WriteFromBlockId(T* t, size_t block_id) {
  ATADriver::GetATADriver().Write(reinterpret_cast<uint8_t*>(t), 1024,
                                  2 * block_id);
}

template <typename T>
void GetArrayFromBlockId(T* t, size_t num, size_t block_id) {
  // sector size is 512 bytes. That means, 1 block spans 2 sectors.
  ATADriver::GetATADriver().Read(reinterpret_cast<uint8_t*>(t), sizeof(T) * num,
                                 2 * block_id);
}

[[maybe_unused]] void PrintInodeInfo(const Ext2Inode& inode) {
  kprintf("Inode Block Info ----------- \n");
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

[[maybe_unused]] void ParseDirectory(uint8_t* buf, int num_read) {
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

class BlockIterator {
 public:
  BlockIterator(const Ext2Inode& inode) : inode_(inode) {}

  BlockIterator& operator++() {
    ++current_index_;
    current_pos += kBlockSize;

    return *this;
  }

  Block operator*() {
    // First check the cache. If it is stale, then it will read from the disk.
    FillCache();

    kprintf("index : ");
    for (size_t i = 0; i <= current_index_.CurrentDepth(); i++) {
      kprintf("%d ", current_index_[i]);
    }
    kprintf("\n");
    // Now returns the cache.
    Block block;
    block.CopyFrom(cached_blocks_[current_index_.CurrentDepth()].data);
    return block;
  }

  size_t Pos() const { return current_pos; }

  void SetOffset(size_t offset) {
    current_pos = (offset / kBlockSize) * kBlockSize;
    current_index_ = CreateBlockIndiceFromOffset(offset);
  }

 private:
  void FillCache() {
    size_t cache_fill_start = 0;
    for (cache_fill_start = 0;
         cache_fill_start <= current_index_.CurrentDepth();
         cache_fill_start++) {
      if (cached_blocks_[cache_fill_start].block_addr !=
          current_index_[cache_fill_start]) {
        break;
      }
    }

    // We should start filling from cache_fill_start.
    for (size_t current_fill = cache_fill_start;
         current_fill <= current_index_.CurrentDepth(); current_fill++) {
      // If this is a first depth, we can just read block address from inode.
      if (current_fill == 0) {
        cached_blocks_[0].block_addr = inode_.block[current_index_[0]];
        GetFromBlockId(&cached_blocks_[0].data, cached_blocks_[0].block_addr);
      } else {
        // We should read the block adderss from the previous block.
        cached_blocks_[current_fill].block_addr =
            cached_blocks_[current_fill - 1].data[current_index_[current_fill]];
        GetFromBlockId(&cached_blocks_[current_fill].data,
                       cached_blocks_[current_fill].block_addr);
      }
    }
  }

  class BlockIndices {
   public:
    BlockIndices() {
      current_depth_ = 0;
      block_index_.fill(0);
    }

    BlockIndices(int* indices, size_t current_depth)
        : current_depth_(current_depth) {
      for (size_t i = 0; i <= current_depth; i++) {
        block_index_[i] = indices[i];
      }
    }

    BlockIndices(const BlockIndices& b) {
      current_depth_ = b.current_depth_;
      for (size_t i = 0; i < 4; i++) {
        block_index_[i] = b.block_index_.at(i);
      }
    }

    BlockIndices& operator=(const BlockIndices& b) {
      current_depth_ = b.current_depth_;
      for (size_t i = 0; i < 4; i++) {
        block_index_[i] = b.block_index_.at(i);
      }

      return *this;
    }

    size_t operator[](size_t i) {
      if (i > current_depth_) {
        PANIC();
      }
      return block_index_[i];
    }

    size_t CurrentDepth() const { return current_depth_; }

    BlockIndices& operator++() {
      RecursiveIndexIncrease(current_depth_);
      return *this;
    }
    bool operator==(const BlockIndices& b) const {
      if (current_depth_ != b.current_depth_) {
        return false;
      }
      for (size_t i = 0; i <= current_depth_; i++) {
        if (block_index_.at(i) != b.block_index_.at(i)) {
          return false;
        }
      }
      return true;
    }
    bool operator!=(const BlockIndices& b) const { return !(operator==(b)); }

   private:
    void RecursiveIndexIncrease(int current_depth) {
      if (current_depth == 0) {
        if (block_index_[0] >= 11) {
          current_depth_++;
        }
        block_index_[0]++;

        return;
      }

      if (block_index_[current_depth] == kMaxBlockEntryInBlock - 1) {
        block_index_[current_depth] = 0;
        RecursiveIndexIncrease(current_depth - 1);
      } else {
        block_index_[current_depth]++;
      }
    }

    // 0 : direct block.
    // 1 : single indirect block.
    // 2 : doubly indirect block.
    // 3 : triply indirect block.
    size_t current_depth_;

    // Index of each block at each level. E.g if this block is double indirect
    // block, then it can be {13, 4, 5}
    //
    // File Inode: block[13] = 10.
    // At block 10, we are at block[4] = 30.
    // At block 30, we are at block[5] = 50.
    std::array<uint32_t, 4> block_index_;
  };

  BlockIndices CreateBlockIndiceFromOffset(size_t offset) {
    constexpr size_t direct_block_limit = kBlockSize * 12;
    constexpr size_t single_block_limit =
        direct_block_limit + kBlockSize * kMaxBlockEntryInBlock;
    constexpr size_t double_block_limit =
        single_block_limit +
        kBlockSize * kMaxBlockEntryInBlock * kMaxBlockEntryInBlock;
    constexpr size_t triple_block_limit =
        double_block_limit + kBlockSize * kMaxBlockEntryInBlock *
                                 kMaxBlockEntryInBlock * kMaxBlockEntryInBlock;
    constexpr size_t single_block_entry_offset =
        kBlockSize * kMaxBlockEntryInBlock;
    constexpr size_t double_block_entry_offset =
        kBlockSize * kMaxBlockEntryInBlock * kMaxBlockEntryInBlock;

    int block_index[4], current_depth = 0;

    if (offset < direct_block_limit) {
      current_depth = 0;
      block_index[0] = offset / kBlockSize;
    } else if (offset < single_block_limit) {
      current_depth = 1;
      block_index[0] = 12;
      block_index[1] = (offset - direct_block_limit) / kBlockSize;
    } else if (offset < double_block_limit) {
      current_depth = 2;
      block_index[0] = 13;
      block_index[1] =
          (offset - single_block_limit) / single_block_entry_offset;
      block_index[2] = (offset - single_block_limit -
                        block_index[1] * single_block_entry_offset) /
                       kBlockSize;
    } else if (offset < triple_block_limit) {
      current_depth = 3;
      block_index[0] = 14;
      block_index[1] =
          (offset - single_block_limit) / double_block_entry_offset;

      size_t second_base = offset - single_block_limit -
                           block_index[1] * double_block_entry_offset;
      block_index[2] = second_base / single_block_entry_offset;

      size_t third_base =
          second_base - block_index[2] * single_block_entry_offset;
      block_index[3] = third_base / kBlockSize;
    } else {
      kprintf("Offset too large! %x \n", offset);
    }

    return BlockIndices(block_index, current_depth);
  }

  void ExpandFileTo(size_t current_file_size, size_t expand_file_size) {
    ASSERT(current_file_size != 0);

    auto prev = CreateBlockIndiceFromOffset(current_file_size - 1);
    auto curr = ++prev;
    auto end = CreateBlockIndiceFromOffset(expand_file_size - 1);

    if (prev == end) {
      return;
    }

    while (true) {
      for (size_t i = 0; i < prev.CurrentDepth(); i++) {
        if (prev[i] != curr[i]) {
        }
      }
      if (curr == end) {
        break;
      }

      prev = curr;
      ++curr;
    }
  }

  const Ext2Inode& inode_;
  size_t current_pos = 0;

  BlockIndices current_index_;

  // Block that only contains the block address. Used by doubly and trebly
  // address blocks.
  using AddressBlock = std::array<uint32_t, kMaxBlockEntryInBlock>;

  struct CachedBlock {
    uint32_t block_addr = -1;
    AddressBlock data;
  };

  std::array<CachedBlock, 4> cached_blocks_;
};

}  // namespace

Ext2FileSystem::Ext2FileSystem() {
  GetFromBlockId(&super_block_, 1);

  num_block_desc_ = integer_ratio_round_up(super_block_.blocks_count,
                                           super_block_.blocks_per_group);
  block_descs_ = (Ext2BlockGroupDescriptor*)kmalloc(
      sizeof(Ext2BlockGroupDescriptor) * num_block_desc_);
  GetArrayFromBlockId(block_descs_, num_block_desc_, 2);

  ASSERT((1024 << super_block_.log_block_size) == kBlockSize);
  ASSERT(super_block_.inode_size == sizeof(Ext2Inode));

  std::array<Ext2Inode, 2> inode_table;
  GetFromBlockId(&inode_table, block_descs_[0].inode_table);

  root_inode_ = inode_table[1];
  root_dir_ = ParseDirectory(root_inode_);

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

  for (size_t i = 10; i < 12; i++) {
    uint8_t buf[1025];
    ReadFile("/c.txt", buf, 1024, i * 1024);
    buf[1024] = '\0';
    kprintf("%s\n", buf);
  }
}

size_t Ext2FileSystem::ReadFile(string_view path, uint8_t* buf, size_t num_read,
                                size_t offset) {
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
  ReadFile(file, buf, num_actually_read, offset);

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

void Ext2FileSystem::ReadFile(const Ext2Inode& file_inode, uint8_t* buf,
                              size_t num_read, size_t offset) {
  BlockIterator iter(file_inode);
  iter.SetOffset(offset);

  size_t start_block_id = offset / kBlockSize;
  size_t end_block_id;

  if ((offset + num_read) % kBlockSize == 0) {
    end_block_id = (offset + num_read) / kBlockSize;
  } else {
    end_block_id = (offset + num_read) / kBlockSize + 1;
  }

  kprintf("s : %d %d %d %d\n", start_block_id, end_block_id, offset,
          offset + num_read);
  size_t read = 0;
  for (; start_block_id < end_block_id; start_block_id++) {
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
}

/*
void Ext2FileSystem::WriteFile(const Ext2Inode& file_inode, uint8_t* buf,
                               size_t num_write, size_t offset) {
}
*/
size_t Ext2FileSystem::GetEmptyBlock() {
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
  for (size_t i = 0; i < inode_bitmap.size(); i++) {
    BitmapInfo& inode_info = inode_bitmap[i];
    int index_in_group = inode_info.bitmap.GetEmptyBitIndex();
    if (index_in_group != -1) {
      kprintf("index : (%d) %lx ", i, index_in_group);
      return index_in_group + super_block_.inodes_per_group * i;
    }
  }
  return 0;
}

void Ext2FileSystem::MarkEmptyInodeAsUsed(size_t inode_num) {
  size_t inode_group_index = inode_num / super_block_.inodes_per_group;
  BitmapInfo& inode_info = inode_bitmap[inode_group_index];
  inode_info.bitmap.FlipBit(inode_num % super_block_.inodes_per_group);

  WriteFromBlockId(inode_info.bitmap.GetBitmap(), inode_info.bitmap_block_id);
}

FileInfo Ext2FileSystem::Stat(string_view path) {
  int inode_num = GetInodeNumberFromPath(path);
  if (inode_num == -1) {
    kprintf("File is not found \n");
    return FileInfo{};
  }

  Ext2Inode file = ReadInode(inode_num);

  FileInfo info;
  info.file_size = file.size;
  info.inode = inode_num;

  return info;
}

std::vector<Ext2Directory> Ext2FileSystem::ParseDirectory(
    const Ext2Inode& dir) {
  // Read the entire directory.
  uint8_t* dir_data = reinterpret_cast<uint8_t*>(kmalloc(dir.size));
  ReadFile(dir, dir_data, dir.size);

  std::vector<Ext2Directory> dir_info;

  size_t current = 0;
  uint8_t* current_read = dir_data;

  while (current < dir.size) {
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

    dir_info.push_back(dir_entry);

    current_read = dir_start + entry_size;
    current += entry_size;
  }

  return dir_info;
}

int Ext2FileSystem::GetInodeNumberFromPath(string_view path) {
  if (path.empty()) {
    PANIC();
  }

  // TODO Support relative paths.
  ASSERT(path[0] == '/');

  size_t current = 1;
  std::vector<Ext2Directory> current_dir = root_dir_;

  while (true) {
    size_t end = path.find_first_of('/', current);
    if (end != npos) {
      string_view name = path.substr(current, end - current);

      bool found = false;
      for (const auto& file : current_dir) {
        if (file.name == name) {
          // Case for /.../.../name/
          if (end == path.size() - 1) {
            return file.inode;
          } else {
            // Case for /.../name/...
            current_dir = ParseDirectory(ReadInode(file.inode));
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
      string_view name = path.substr(current);
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

}  // namespace Kernel
