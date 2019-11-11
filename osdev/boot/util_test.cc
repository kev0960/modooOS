#include "kernel_test.h"
#include "list.h"
#include "vector.h"

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

TEST(KernelListTest, RemoveSelf) {
  KernelList<int> list;
  INSERT_FRONT(1)
  INSERT_FRONT(2)
  INSERT_FRONT(3)

  elem1.RemoveSelfFromList();
  EXPECT_EQ(list.size(), 2u);

  EXPECT_EQ(*list.begin(), 3);
  EXPECT_EQ(*++list.begin(), 2);

  elem2.RemoveSelfFromList();
  EXPECT_EQ(list.size(), 1u);
  EXPECT_EQ(*list.begin(), 3);

  elem3.RemoveSelfFromList();
  EXPECT_EQ(list.size(), 0u);
}

TEST(KernelListTest, RemoveSelfMore) {
  KernelList<int> list;
  INSERT_FRONT(1)
  INSERT_FRONT(2)
  INSERT_FRONT(3)
  INSERT_FRONT(4)

  elem2.RemoveSelfFromList();
  EXPECT_EQ(list.size(), 3u);

  EXPECT_EQ(*list.begin(), 4);
  EXPECT_EQ(*++list.begin(), 3);
  EXPECT_EQ(*++++list.begin(), 1);

  elem3.RemoveSelfFromList();
  EXPECT_EQ(list.size(), 2u);
  EXPECT_EQ(*list.begin(), 4);
  EXPECT_EQ(*++list.begin(), 1);

  elem4.RemoveSelfFromList();
  EXPECT_EQ(list.size(), 1u);
  EXPECT_EQ(*list.begin(), 1);

  elem1.RemoveSelfFromList();
  EXPECT_EQ(list.size(), 0u);
}

TEST(KernelVectorTest, Basic) {
  std::vector<int> vec;
  vec.push_back(0);
  vec.push_back(1);
  vec.push_back(2);
  vec.push_back(3);
  vec.push_back(4);

  for (int i = 0; i < 5; i++) {
    EXPECT_EQ(vec[i], i);
  }
}

static int copy_const_call = 0;

struct A {
  A() { kprintf("Default! \n"); }
  A(const A&) {
    kprintf("Copy construct! \n");
    copy_const_call++;
  }
  A(A&&) { kprintf("Move construct! \n"); }
  ~A() { kprintf("Destruct! \n"); }
};

TEST(KernelVectorTest, CheckMoveCalled) {
  std::vector<A> vec;
  vec.push_back(A{});
  vec.push_back(A{});
  vec.push_back(A{});
  vec.push_back(A{});

  EXPECT_EQ(copy_const_call, 0);
}

struct B {
  B() { kprintf("Default! \n"); }
  B(const B&) { kprintf("Copy construct! \n"); }
  ~B() { kprintf("Destruct! \n"); }
};

TEST(KernelVectorTest, CheckCopyCalled) {
  std::vector<B> vec;
  vec.push_back(B{});
  vec.push_back(B{});
  vec.push_back(B{});
  vec.push_back(B{});
}

}  // namespace kernel_test
}  // namespace Kernel
