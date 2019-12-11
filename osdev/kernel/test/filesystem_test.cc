#include "../kernel/ext2.h"
#include "kernel_test.h"
#include "../kernel/kthread.h"
#include "string_util.h"

namespace Kernel {
namespace kernel_test {

static const char* kTest1FilePath = "/test/filesystem_test/test_1.txt";

TEST(FilesystemTest, SimpleReadFile) {
  auto& ext2 = Ext2FileSystem::GetExt2FileSystem();

  uint8_t buf[100];
  int file_size = ext2.Stat(kTest1FilePath).file_size;

  size_t num_read = ext2.ReadFile(kTest1FilePath, buf, file_size);
  buf[num_read] = 0;
}

void read() {
  auto& ext2 = Ext2FileSystem::GetExt2FileSystem();
  for (int i = 0; i < 1000; i++) {
    uint8_t buf[100];
    int file_size = ext2.Stat(kTest1FilePath).file_size;

    size_t num_read = ext2.ReadFile(kTest1FilePath, buf, file_size);
    buf[num_read] = 0;

    EXPECT_EQ(string_view{(char*)buf}, string_view{"this is some data\n"});
  }
}

TEST(FilesystemTest, MultipleRead) {
  KernelThread thread1(read);
  KernelThread thread2(read);
  KernelThread thread3(read);

  thread1.Start();
  thread2.Start();
  thread3.Start();

  thread1.Join();
  thread2.Join();
  thread3.Join();
}

}  // namespace kernel_test
}  // namespace Kernel
