#include "../std/set.h"
#include "kernel_test.h"

namespace Kernel {
namespace kernel_test {

TEST(TreeTest, SimpleInsert) {
  std::BinaryTreeNode<int> node1(2);
  std::BinaryTreeNode<int> node2(1);
  std::BinaryTreeNode<int> node3(3);

  EXPECT_TRUE(node1.NumChild() == 0);

  node1.SetLeft(&node2);
  EXPECT_TRUE(node1.NumChild() == 1);

  node1.SetRight(&node3);
  EXPECT_TRUE(node1.NumChild() == 2);

  node1.RemoveChild(&node2);
  EXPECT_TRUE(node1.Left() == nullptr);
  EXPECT_TRUE(node1.NumChild() == 1);

  EXPECT_EQ(node1.GetOnlyChild(), &node3);

  EXPECT_EQ(node1.AddChild(&node2), nullptr);
  EXPECT_EQ(node1.Left(), &node2);
}

template <typename T>
bool CheckExist(std::set<T>& s, T* data, int num_data) {
  for (int i = 0; i < num_data; i++) {
    if (s.count(data[i]) == 0) {
      kprintf("Not found! %d \n", data[i]);
      return false;
    }
  }
  return true;
}

TEST(SetTest, SimpleInsert) {
  std::set<int> s;
  s.insert(2);
  s.insert(1);
  s.insert(3);
  s.insert(6);
  s.insert(5);
  s.insert(4);
  s.insert(8);

  s.Print();
  int data[] = {1, 2, 3, 4, 5, 6, 8};
  for (int i = 0; i < 7; i++) {
    EXPECT_TRUE(s.count(data[i]) == 1);
  }
  EXPECT_TRUE(s.count(7) == 0);
  EXPECT_TRUE(s.count(0) == 0);
}

TEST(SetTest, SimpleEraseTest) {
  std::set<int> s;
  s.insert(2);
  s.insert(1);
  s.insert(3);

  s.erase(1);
  EXPECT_TRUE(s.count(1) == 0);
  EXPECT_TRUE(s.count(2) == 1);
  EXPECT_TRUE(s.count(3) == 1);

  s.erase(2);
  EXPECT_TRUE(s.count(1) == 0);
  EXPECT_TRUE(s.count(2) == 0);
  EXPECT_TRUE(s.count(3) == 1);

  s.erase(3);
  EXPECT_TRUE(s.count(1) == 0);
  EXPECT_TRUE(s.count(2) == 0);
  EXPECT_TRUE(s.count(3) == 0);
}

//           2
//        1       4
//            3      10
//                 6    12
//                   7
TEST(SetTest, EraseTestCase1) {
  std::set<int> s;
  s.insert(2);
  s.insert(1);
  s.insert(4);
  s.insert(3);
  s.insert(10);
  s.insert(6);
  s.insert(12);
  s.insert(7);

  int data1[] = {1, 2, 3, 4, 6, 7, 10, 12};
  EXPECT_TRUE(CheckExist(s, data1, 8));

  s.erase(4);

  int data2[] = {1, 2, 3, 6, 7, 10, 12};
  EXPECT_TRUE(CheckExist(s, data2, 7));

  s.erase(2);
  int data3[] = {1, 3, 6, 7, 10, 12};
  EXPECT_TRUE(CheckExist(s, data3, 6));

  s.erase(6);
  int data4[] = {1, 3, 7, 10, 12};
  EXPECT_TRUE(CheckExist(s, data4, 5));

  s.erase(1);
  int data5[] = {3, 7, 10, 12};
  EXPECT_TRUE(CheckExist(s, data5, 4));

  s.erase(12);
  int data6[] = {3, 7, 10};
  EXPECT_TRUE(CheckExist(s, data6, 3));
}

}  // namespace kernel_test
}  // namespace Kernel
