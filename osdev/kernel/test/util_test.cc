#include "../kernel/kernel_list.h"
#include "../std/algorithm.h"
#include "../std/bitmap.h"
#include "../std/list.h"
#include "../std/vector.h"
#include "kernel_test.h"

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
  A() {}
  A(const A&) { copy_const_call++; }
  A(A&&) {}
  ~A() {}
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
  B(size_t data) : data(data) {}
  B(const B& b) : data(b.data) {}
  ~B() {}

  size_t data;
};

TEST(KernelVectorTest, CheckCopyCalled) {
  std::vector<B> vec;
  vec.push_back(B{1});
  vec.push_back(B{2});
  vec.push_back(B{3});
  vec.push_back(B{4});

  for (size_t i = 0; i < vec.size(); i++) {
    EXPECT_EQ(vec[i].data, i + 1);
  }
}

TEST(KernelVectorTest, CopyVector) {
  std::vector<B> vec;
  vec.reserve(4);

  vec.push_back(B{1});
  vec.push_back(B{2});
  vec.push_back(B{3});
  vec.push_back(B{4});

  std::vector<B> vec2;
  vec2.reserve(10);
  vec.push_back(B{5});
  vec.push_back(B{6});
  vec.push_back(B{7});

  for (size_t i = 0; i < vec2.size(); i++) {
    EXPECT_EQ(vec2[i].data, i + 5);
  }

  vec2 = vec;

  for (size_t i = 0; i < vec2.size(); i++) {
    EXPECT_EQ(vec2[i].data, i + 1);
  }
}

TEST(KernelBitmapTest, Bitmap) {
  Bitmap<64> bitmap;
  *bitmap.GetBitmap() = 0;

  EXPECT_EQ(bitmap.GetEmptyBitIndex(), 0);
  EXPECT_FALSE(bitmap.IsSet(0));
  bitmap.FlipBit(0);
  EXPECT_TRUE(bitmap.IsSet(0));

  EXPECT_EQ(bitmap.GetEmptyBitIndex(), 1);
  bitmap.FlipBit(1);
  bitmap.FlipBit(2);
  bitmap.FlipBit(3);
  EXPECT_EQ(bitmap.GetEmptyBitIndex(), 4);

  bitmap.FlipBit(0);
  EXPECT_EQ(bitmap.GetEmptyBitIndex(), 0);
}

TEST(KernelBitmapTest, BitmapLarge) {
  Bitmap<256> bitmap;
  for (int i = 0; i < 4; i++) {
    bitmap.GetBitmap()[i] = 0xFFFFFFFFFFFFFFFF;
  }

  EXPECT_EQ(bitmap.GetEmptyBitIndex(), -1);
  bitmap.FlipBit(123);
  EXPECT_EQ(bitmap.GetEmptyBitIndex(), 123);
}

TEST(AlgorithmTest, LowerBound) {
  std::vector<int> vec;
  vec.push_back(1);
  vec.push_back(3);
  vec.push_back(4);
  vec.push_back(7);
  vec.push_back(8);
  vec.push_back(11);
  vec.push_back(13);

  EXPECT_EQ(*std::lower_bound(vec.begin(), vec.end(), 5), 7);
  EXPECT_EQ(*std::lower_bound(vec.begin(), vec.end(), 8), 8);
}

TEST(AlgorithmTest, UpperBound) {
  std::vector<int> vec;
  vec.push_back(1);
  vec.push_back(3);
  vec.push_back(4);
  vec.push_back(7);
  vec.push_back(8);
  vec.push_back(11);
  vec.push_back(13);

  EXPECT_EQ(*std::upper_bound(vec.begin(), vec.end(), 5), 7);
  EXPECT_EQ(*std::upper_bound(vec.begin(), vec.end(), 8), 11);
}

}  // namespace kernel_test
}  // namespace Kernel
