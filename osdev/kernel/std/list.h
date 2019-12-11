#ifndef LIST_H
#define LIST_H

namespace Kernel {
namespace std {

template <typename T, typename Alloc>
class list {
 public:
  list() : head_(nullptr), tail_(nullptr) {}

  void push_back(const T& t) {}

 private:
  struct ListNode {
    ListNode* prev;
    ListNode* next;
    T t;
  };

  ListNode* head_;
  ListNode* tail_;
};
}  // namespace std
}  // namespace Kernel

#endif
