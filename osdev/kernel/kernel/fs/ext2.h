#ifndef EXT2_H
#define EXT2_H

#include "../../std/bitmap.h"
#include "../../std/string.h"
#include "../../std/types.h"
#include "../../std/vector.h"
#include "ata.h"

namespace Kernel {
// Note that the names and comments of these Ext2 structs are brought from
// http://www.nongnu.org/ext2-doc/ext2.html

// SuperBlock : 1024 bytes.
struct Ext2SuperBlock {
  // total # of inodes, used and free, in system
  uint32_t inodes_count;

  // total # of blocks, used and free, in system
  uint32_t blocks_count;

  // total # of blocks reserved for superuser
  uint32_t reserved_blocks_count;

  // total # of free blocks, including r_blocks_count reserved blocks
  uint32_t free_blocks_count;

  // total # of free inodes
  uint32_t free_inodes_count;

  // First data block location -- i.e., id of the block containing
  // the superblock.  Is 0 for filesystems with block size > 1KB, or
  // 1 for file systems with block size of 1KB.  Remember that the
  // superblock always starts at the 1024th byte on the disk.
  uint32_t first_data_block;

  // log_2(blocksize/1024).  So, blocksize is computed as:
  //
  //   uint32_t blocksize = 1024 << log_block_size;
  uint32_t log_block_size;

  // shift of log_2(fragmentsize/1024).  So, fragmentsize is computed as:
  //
  //   uint32_t fragmentsize =
  //       (s_log_frag_size > 0) ?
  //           (1024 << log_frag_size) : (1024 >> -log_frag_size);
  int32_t log_frag_size;

  // number of blocks per group.  Combined with first_data_block,
  // you can use this to figure out the block group boundaries.
  uint32_t blocks_per_group;

  // number of fragments per group.  Also can be used to determine the
  // size of the block bitmap of each block group.  In ext2, the fragment
  // size is always the same as the block size -- fragmented blocks are
  // not yet implemented.
  uint32_t frags_per_group;

  // number of inodes per group.  Also can be used to determine the
  // size of the inode bitmap of each block group.
  uint32_t inodes_per_group;

  // the last time the filesystem was mounted, in "UNIX time" (# seconds
  // since epoch).
  uint32_t mtime;

  // the last time the file system was written to, in "UNIX time" (#
  // seconds since epoch).
  uint32_t wtime;

  // the # times the file system was mounted since the last time it
  // was verified (i.e., since fsck has been run)
  uint16_t mnt_count;

  // the max # of times the file system may be mounted before
  // a full check is performed
  uint16_t max_mnt_count;

  // a "magic number" identifying the file system as type Ext2.
  // this value is set to EXT2_SUPER_MAGIC, which has value 0xEF53
  uint16_t magic;

  // the file system state -- EXT2_VALID_FS means the filesystem
  // was last unmounted cleanly, while EXT2_ERROR_FS means the
  // filesystem was not unmounted cleanly, and likely has errors
  uint16_t state;

  // ext2 file system version information -- minor revision level
  uint16_t minor_rev_level;

  // the last time a consistency check was run on the filesystem; time
  // is UNIX time, i.e., seconds since the last epoch
  uint32_t lastcheck;

  // the maximum time (in seconds) allowed between filesystem checks
  uint32_t checkinterval;

  // the identifier of the operating system that created this FS.
  // could be EXT2_OS_LINUX, EXT2_OS_HURD, EXT2_OS_MASIC,
  // EXT2_OS_FREEBSD, or EXT2_OS_LITES.
  uint32_t creator_os;

  // ext2 file system version information -- major revision level.
  // could be either EXT2_GOOD_OLD_REV or EXT2_DYNAMIC_REV.
  uint32_t rev_level;

  // the default userID for reserved blocks.  defaults to
  // EXT2_DEF_RESUID, which has value 0 (i.e., root).
  uint16_t def_resuid;

  // the default group ID for reserved blocks.  defaults to
  // EXT2_DEF_RESGID, which has value 0 (i.e., root).
  uint16_t def_resgid;

  // the first, non-reserved inode usable for standard files.
  // in EXT2_GOLD_OLD_REV version, this is set to
  // value 11 (EXT2_GOOD_OLD_FIRST_INO).  In revisions 1 or later,
  // can be anything.
  uint32_t first_ino;

  // the size of an inode.  In revision 0, this is
  // EXT2_GOOD_OLD_INODE_SIZE (128 bytes).  In later revisions, can
  // be any power of 2 less than or equal to the block size
  // (i.e., inode_size <= 1<<s_log_block_size;
  uint16_t inode_size;

  // the block group number hosting this superblock.  can be used
  // to rebuild the filesystem from any superblock backup.
  uint16_t block_group_nr;

  // a 32bit bitmask of compatible features.  By compatible, we
  // mean that the filesystem can support these or not without
  // any risk of damaging metadata.  The features are:
  //
  //  EXT2_FEATURE_COMPAT_DIR_PREALLOC (0x0001):
  //          Block pre-allocation for new directories
  //  EXT2_FEATURE_COMPAT_IMAGIC_INODES (0x0002):
  //          Has "magic" inodes.
  //  EXT3_FEATURE_COMPAT_HAS_JOURNAL (0x0004):
  //          An Ext3 journal exists
  //  EXT2_FEATURE_COMPAT_EXT_ATTR (0x0008):
  //          Extended inode attributes are present
  //  EXT2_FEATURE_COMPAT_RESIZE_INO (0x0010):
  //          Non-standard inode size used
  //  EXT2_FEATURE_COMPAT_DIR_INDEX (0x0020):
  //          Directory indexing (HTree)
  //
  // So, for example, if the filesystem has a journal and extended
  // attributes, then:
  //   feature_compat =  EXT2_FEATURE_COMPAT_HAS_JOURNAL &
  //                       EXT2_FEATURE_COMPAT_EXT_ATTR;
  //
  uint32_t feature_compat;

  // a 32bit bitmask of incompatible features, i.e., if they are
  // present they will affect the filesystem metadata.  The file
  // system implementation should refuse to mount the filesystem if
  // any of the indicated features are unsupported.
  // (steve gribble): I haven't tracked down the meaning of all of
  // these, but the filesystem image we use in 451 has none of them.
  //
  // EXT2_FEATURE_INCOMPAT_COMPRESSION (0x0001):
  //       Disk/File compression is used
  // EXT2_FEATURE_INCOMPAT_FILETYPE (0x0002)
  // EXT3_FEATURE_INCOMPAT_RECOVER (0x0004)
  // EXT3_FEATURE_INCOMPAT_JOURNAL_DEV (0x0008)
  // EXT2_FEATURE_INCOMPAT_META_BG (0x0010)
  //
  uint32_t feature_incompat;

  // a 32bit bitmask of read-only features.  The filesystem should
  // mount as read-only if any of these are unsupported.
  //
  // EXT2_FEATURE_RO_COMPAT_SPARSE_SUPER (0x0001):
  //      sparse superblock
  // EXT2_FEATURE_RO_COMPAT_LARGE_FILE (0x0002):
  //      Large file support, 64-bit file size
  // EXT2_FEATURE_RO_COMPAT_BTREE_DIR (0x0004):
  //      Binary tree sorted directory files
  uint32_t feature_ro_compat;

  // a 128 bit value used as the unique volume ID.  every file system
  // in the world should be given a unique volume ID, if possible.
  uint8_t uuid[16];

  // a null-terminated volume name.  only ISO-Latin-1
  // characters may be used.  this is a rarely used feature.
  uint8_t volume_name[16];

  // the path (null-terminated string) where the filesystem was last
  // mounted.  could be used for auto-finding the mount point.  often
  // is left as the empty string, i.e., not used by some implementations.
  uint8_t last_mounted[64];

  // if this volume is compressed (see feature_compat), this bitfield
  // tells you the compression algorithms (yes, plural!) used.
  //
  // EXT2_LZV1_ALG (0x00000001)
  // EXT2_LZRW3A_ALG (0x00000002)
  // EXT2_GZIP_ALG (0x00000004)
  // EXT2_BZIP2_ALG (0x00000008)
  // EXT2_LZO_ALG (0x00000010)
  uint32_t algo_bitmap;

  // 8-bit value representing the number of blocks the implementation
  // should pre-allocate when creating a new regular file.  typically
  // only used with ext4 filesystems.  usually "0" for pre-ext4
  // filesystems.
  uint8_t prealloc_blocks;

  // 8-bit value representing the number of blocks the implementation
  // should pre-allocated when creating a new directory.  again,
  // only really used in ext4, and only if the
  // EXT2_FEATURE_COMPAT_DIR_PREALLOC flag is present in the
  // feature_compat field.
  uint8_t prealloc_dir_blocks;

  // two bytes of padding
  uint16_t padding_1;

  // 16 byte value containing the UUID of the journal superblock, if
  // this filesystem has an ext3 journal.
  uint8_t journal_uuid[16];

  // the 32 bit inode number of the journal file.
  uint32_t journal_inum;

  // the 32-bit device number of the journal file.
  uint32_t journal_dev;

  // a 32-bit inode number, pointing to the first inode in the list
  // of inodes to delete.  (see documentation on ext3 journaling for
  // information -- safe to ignore for non-ext3 systems).
  uint32_t last_orphan;

  // an array of 4 32-bit values containing seeds used for the
  // hash algorithm for directory indexing, if this is used.
  uint32_t hash_seed[4];

  // an 8-bit value containing the default hash version used for
  // directory indexing.
  uint8_t def_hash_version;

  // 3 bytes of padding, reserved for future expansion
  uint8_t padding_2[3];

  // a 32-bit value containing the default mount options for this
  // filesystem.
  uint32_t default_mount_options;

  // a 32-bit value indicating the block group ID of the first
  // meta-block group.  (ext3-only extension, I believe).
  uint32_t first_meta_bg;

  // unused -- reserved for future revisions
  uint8_t unused[760];
};

// 32 bytes.
struct Ext2BlockGroupDescriptor {
  // the 32-bit block ID of the first block of the "block bitmap"
  // for the blockgroup this descriptor represents.
  uint32_t block_bitmap;

  // the 32-bit block ID of the first block of the "inode bitmap"
  // for the blockgroup this descriptor represents.
  uint32_t inode_bitmap;

  // the 32-bit block ID of the first block of the "inode table"
  // for the blockgroup this descriptor represents.
  uint32_t inode_table;

  // the 16bit value indicating the total number of free blocks
  // in the blockgroup this descriptor represents.
  uint16_t free_blocks_count;

  // the 16bit value indicating the total number of free inodes
  // in the blockgroup this descriptor represents.
  uint16_t free_inodes_count;

  // the 16bit value indicating the number of inodes that have
  // been allocated to directories in the blockgroup this descriptor
  // represents.
  uint16_t used_dirs_count;

  // a 16-bit pad value so the structure is 32-bit aligned.
  uint16_t pad;

  // 12 bytes of space reserved for future revisions.
  uint8_t reserved[12];
};

struct Ext2Inode {
  // a 16 bit value used to indicate the format of the file and access
  // rights.  Here are the possible values, which can be combined
  // in various ways:
  //
  //  -- file format --
  //  EXT2_S_IFSOCK   0xC000  socket
  //  EXT2_S_IFLNK    0xA000  symbolic link
  //  EXT2_S_IFREG    0x8000  regular file
  //  EXT2_S_IFBLK    0x6000  block device
  //  EXT2_S_IFDIR    0x4000  directory
  //  EXT2_S_IFCHR    0x2000  character device
  //  EXT2_S_IFIFO    0x1000  fifo
  //  -- process execution user/group override --
  //  EXT2_S_ISUID    0x0800  Set process User ID
  //  EXT2_S_ISGID    0x0400  Set process Group ID
  //  EXT2_S_ISVTX    0x0200  sticky bit
  //  -- access rights --
  //  EXT2_S_IRUSR    0x0100  user read
  //  EXT2_S_IWUSR    0x0080  user write
  //  EXT2_S_IXUSR    0x0040  user execute
  //  EXT2_S_IRGRP    0x0020  group read
  //  EXT2_S_IWGRP    0x0010  group write
  //  EXT2_S_IXGRP    0x0008  group execute
  //  EXT2_S_IROTH    0x0004  others read
  //  EXT2_S_IWOTH    0x0002  others write
  //  EXT2_S_IXOTH    0x0001  others execute
  //
  uint16_t mode;

  // 16-bit user id associated with the file.
  uint16_t uid;

  // 32-bit value indicating the size of the file in bytes
  // (revision 0).  In revision 1 and later, this represents the lower
  // 32 bits of the file size (upper 32 bits are in dir_acl).
  uint32_t size;

  // 32-bit value representing the unix time (# of seconds since the
  // epoch, january 1st 1970) of the last time this inode was
  // accessed.
  uint32_t atime;

  // 32-bit value representing the unix time (# of seconds since the
  // epoch, january 1st 1970) of when this inode was created.
  uint32_t ctime;

  // 32-bit value representing the unix time (# of seconds since the
  // epoch, january 1st 1970) of the last time this inode was
  // modified.
  uint32_t mtime;

  // 32-bit value representing the unix time (# of seconds since the
  // epoch, january 1st 1970) of when this inode was deleted.
  uint32_t dtime;

  // 16-bit value of the group id having access to this file.
  uint16_t gid;

  // 16-bit value indicating how many times this inode is linked
  // (i.e., referred to).  Most files will have a link count of
  // 1. Files with hard links pointing to them will have an additional
  // count for each hard link.  Symbolic links do not affect the link
  // count of an inode.  When the link count reaches 0, the inode and
  // all of its associated blocks are deleted.
  uint16_t links_count;

  // 32-bit value representing the total # of blocks reserved to
  // contain data of this inode, regardless of whether these blocks
  // are used.  The block numbers of these are contained in the
  // block array.
  uint16_t blocks;

  // 32-bit value indicating flags controlling how the filesystem
  // should access the data of this node.
  //
  // To see the flags that are defined, and their values and meaning,
  // refer to this web page:
  //     http://www.nongnu.org/ext2-doc/ext2.html#I-FLAGS
  //
  uint32_t flags;

  // 32-bit value that is os-dependent.  in linux, is reserved.
  // this looks complicated, but a union is simple -- it defines
  // multiple different ways to represent the same region of
  // the structure.  So, you could reference the field in any
  // of the following ways, all of which refer to the same
  // 32 bit value in this structure:
  //   (variable).i_osdi1.linux1.l_i_reserved1;
  union {
    struct {
      uint32_t l_i_reserved1;
    } linux1;
    struct {
      uint32_t h_i_translator;
    } hurd1;
    struct {
      uint32_t m_i_reserved1;
    } masix1;
  } osdi1;

  // an array of 15 32-bit blocknumbers pointing to the blocks
  // containing data.  The first 12 entries in the array
  // (i_block[0]-iblock[11]) contain the block numbers of data blocks
  // (i.e., are "direct" entries). The 13th entry (i_block[12])
  // contains the block number of the first indirect block, which
  // itself contains (blocksize / 4) block numbers of data blocks.
  // The 14th entry (i_block[13]) contains the block number of
  // the first doubly-indirect block.  The 15th entry (i_block[14]) is
  // the block number of the first trebly-indirect block.
  uint32_t block[15];

  // 32-bit value used to indicate the file version (used by NFS).
  uint32_t generation;

  // 32-bit value indicating the block number containing extended
  // attributes.  With revision 0 of ext2 (which we're using in this
  // class), this value is always 0.
  uint32_t file_acl;

  // 32-bit value.  With revision 0 of ext2, this is always 0.
  // With revision 1, it contains the high 32 bits of the 64bit file
  // size.
  uint32_t dir_acl;

  // 32-bit value indicating the location of the file fragment.
  // in linux, fragments are unsupported, so this value is always 0.
  uint32_t faddr;

  // the next 96-bits are os-dependant structures.   We don't
  // really have to worry about these, since the linux version
  // is about fragmentation, and revision 0 of ext2 does not
  // use fragmentation.  So, this is safe to ignore.
  union {
    struct {
      uint8_t l_i_frag;  /* Fragment number */
      uint8_t l_i_fsize; /* Fragment size */
      uint16_t pad1;
      uint16_t l_i_uid_high; /* these 2 fields    */
      uint16_t l_i_gid_high; /* were reserved2[0] */
      uint32_t l_i_reserved2;
    } linux2;
    struct {
      uint8_t h_i_frag;  /* Fragment number */
      uint8_t h_i_fsize; /* Fragment size */
      uint16_t h_i_mode_high;
      uint16_t h_i_uid_high;
      uint16_t h_i_gid_high;
      uint32_t h_i_author;
    } hurd2;
    struct {
      uint8_t m_i_frag;  /* Fragment number */
      uint8_t m_i_fsize; /* Fragment size */
      uint16_t m_pad1;
      uint32_t m_i_reserved2[2];
    } masix2;
  } osd2;
};

struct Ext2Directory {
  uint32_t inode;
  uint8_t file_type;
  KernelString name;
};

struct linux_dirent {
  uint32_t d_ino;     /* Inode number */
  uint32_t d_off;     /* Offset to next linux_dirent */
  uint16_t d_reclen; /* Length of this linux_dirent */
  char d_name[];           /* Filename (null-terminated) */
                           /* length is actually (d_reclen - 2 -
                              offsetof(struct linux_dirent, d_name) */
};

struct FileInfo {
  size_t inode;      // Inode number.
  size_t file_size;  // File size in bytes.
  uint16_t mode;     // File info
};

class Ext2FileSystem {
 public:
  Ext2FileSystem(const Ext2FileSystem&) = delete;
  Ext2FileSystem operator=(const Ext2FileSystem&) = delete;

  static Ext2FileSystem& GetExt2FileSystem() {
    static Ext2FileSystem ext2;
    return ext2;
  }

  //  -- file format --
  //  EXT2_S_IFSOCK   0xC000  socket
  //  EXT2_S_IFLNK    0xA000  symbolic link
  //  EXT2_S_IFREG    0x8000  regular file
  //  EXT2_S_IFBLK    0x6000  block device
  //  EXT2_S_IFDIR    0x4000  directory
  //  EXT2_S_IFCHR    0x2000  character device
  //  EXT2_S_IFIFO    0x1000  fifo

  enum Ext2FileMode {
    S_SOCK,
    S_LNK,
    S_REG,
    S_BLK,
    S_DIR,
    S_CHR,
    S_FIFO,
    S_UNKNOWN
  };

  static Ext2FileMode GetFileFormatFromMode(uint16_t mode) {
    Ext2FileMode modes[] = {S_SOCK, S_LNK, S_REG, S_BLK, S_DIR, S_CHR, S_FIFO};
    int values[] = {0xC000, 0xA000, 0x8000, 0x6000, 0x4000, 0x2000, 0x1000};

    for (int i = 0; i < 7; i++) {
      if ((mode & values[i]) == values[i]) {
        return modes[i];
      }
    }

    return S_UNKNOWN;
  }

  static KernelString GetAbsolutePath(const KernelString& path,
                                      const KernelString& current_dir);

  // Generate shortest-form absolute path from the absolute path.
  // E.g /a/../b --> /b
  static KernelString GetCanonicalAbsolutePath(
      const KernelString& absolute_path);

  // Read the file at the path.
  size_t ReadFile(std::string_view path, uint8_t* buf, size_t num_read,
                  size_t offset = 0);
  // Write to the file at the path.
  void WriteFile(std::string_view path, uint8_t* buf, size_t num_write,
                 size_t offset = 0);

  Ext2Inode ReadInode(size_t inode_addr);
  void WriteInode(size_t inode_addr, const Ext2Inode& inode);

  size_t ReadFile(Ext2Inode* file_inode, uint8_t* buf, size_t num_read,
                  size_t offset = 0);
  void WriteFile(size_t inode_num, uint8_t* buf, size_t num_write,
                 size_t offset = 0);

  // Get file info.
  FileInfo Stat(std::string_view path);
  FileInfo Stat(size_t inode_num);

  bool CreateFile(std::string_view path, bool is_directory);

  int GetInodeNumberFromPath(std::string_view path);

  std::vector<Ext2Directory> ParseDirectory(Ext2Inode* dir);

 private:
  struct BitmapInfo {
    size_t bitmap_block_id;
    Bitmap<1024 * 8> bitmap;
  };

  Ext2FileSystem();

  // Get empty block. returns the block number.
  size_t GetEmptyBlock();
  void MarkEmptyBlockAsUsed(size_t block_id);

  // Returns the inode number.
  size_t GetEmptyInode();
  void MarkEmptyInodeAsUsed(size_t inode_num);

  void ExpandFileSize(size_t inode_num, size_t expanded_file_size);

  size_t GetEndOfDirectoryEntry(Ext2Inode* dir_inode);

  Ext2SuperBlock super_block_;
  Ext2Inode root_inode_;

  std::vector<Ext2Directory> root_dir_;

  Ext2BlockGroupDescriptor* block_descs_;
  size_t num_block_desc_;

  std::vector<BitmapInfo> block_bitmap;
  std::vector<BitmapInfo> inode_bitmap;

  MultiCoreSpinLock fs_lock_;
};

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

}  // namespace Kernel

#endif
