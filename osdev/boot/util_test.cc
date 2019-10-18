#include "kernel_test.h"
#include "list.h"

#define INSERT_FRONT(NUM)                  \
  KernelListElement<int> elem##NUM(&list); \
  elem##NUM.PushFront();                   \
  elem##NUM.Set(NUM);

#define INSERT_BACK(NUM)                   \
  KernelListElement<int> elem##NUM(&list); \
  elem##NUM.PushBack();                    \
  elem##NUM.Set(NUM);

namespace Kernel {
namespace kernel_test {

TEST(KernelListTest, SimplePushFront) {
  KernelList<int> list;
  INSERT_FRONT(1)
  INSERT_FRONT(2)
  INSERT_FRONT(3)

  int result[] = {3, 2, 1};
  auto itr = list.begin();
  for (int i = 0; i < 3; i++) {
    EXPECT_EQ(*itr++, result[i]);
  }
}

TEST(KernelListTest, SimplePushBack) {
  KernelList<int> list;
  INSERT_BACK(1)
  INSERT_BACK(2)
  INSERT_BACK(3)

  int result[] = {1, 2, 3};
  auto itr = list.begin();
  for (int i = 0; i < 3; i++) {
    EXPECT_EQ(*itr++, result[i]);
  }
}

TEST(KernelListTest, ZigZag) {
  KernelList<int> list;
  INSERT_FRONT(1)
  INSERT_BACK(2)
  INSERT_FRONT(3)
  INSERT_BACK(4)
  INSERT_FRONT(5)
  INSERT_BACK(6)

  int result[] = {5, 3, 1, 2, 4, 6};
  auto itr = list.begin();
  for (int i = 0; i < 6; i++) {
    EXPECT_EQ(*itr++, result[i]);
  }
}

TEST(KernelListTest, PopFront) {
  KernelList<int> list;
  INSERT_FRONT(1)
  INSERT_FRONT(2)
  INSERT_FRONT(3)

  EXPECT_EQ(list.size(), 3u);
  int result[] = {3, 2, 1};
  for (int i = 0; i < 3; i++) {
    EXPECT_EQ(*list.begin(), result[i]);
    EXPECT_EQ(list.pop_front()->Get(), result[i]);
  }
  EXPECT_TRUE(list.empty());
}

TEST(KernelListTest, PopBack) {
  KernelList<int> list;
  INSERT_FRONT(1)
  INSERT_FRONT(2)
  INSERT_FRONT(3)

  int result[] = {1, 2, 3};
  for (int i = 0; i < 3; i++) {
    EXPECT_EQ(list.pop_back()->Get(), result[i]);
  }
  EXPECT_TRUE(list.empty());
}

}  // namespace kernel_test
}  // namespace Kernel
