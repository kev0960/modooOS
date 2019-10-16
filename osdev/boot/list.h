#ifndef LIST_H
#define LIST_H

#include "types.h"

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

// List that does not require dynamic allocation.
template <typename T>
class KernelList;

template <typename T>
class KernelListElement {
 public:
  KernelListElement(KernelList<T>* stack_list)
      : prev(nullptr), next(nullptr), stack_list_(stack_list) {}

  void PushFront() {
    RemoveSelfFromList();
    stack_list_->PushFront(this);
  }
  void PushBack() {
    RemoveSelfFromList();
    stack_list_->PushBack(this);
  }

  // Remove self from the list.
  void RemoveSelfFromList() {
    if (prev) {
      prev->next = next;
    }
    if (next) {
      next->prev = prev;
    }

    prev = next = nullptr;
  }

  KernelListElement* prev;
  KernelListElement* next;

 private:
  T t;

  KernelList<T>* stack_list_;
};

template <typename T>
class KernelList {
 public:
  KernelList() : head_(nullptr), tail_(nullptr) {}

  bool IsEmpty() const { return head_ == nullptr; }
  size_t GetSize() const { return size_; }

  void PushFront(KernelListElement<T>* elem) {
    size_++;

    // If empty.
    if (IsEmpty()) {
      head_ = tail_ = elem;
      elem->prev = elem->next = nullptr;
    } else {
      // Link previous head and new head together.
      head_->prev = elem;
      elem->next = head_;

      // There is nothing on the left side of new head.
      elem->prev = nullptr;

      // Set head point to new element.
      head_ = elem;
    }
  }

  void PushBack(KernelListElement<T>* elem) {
    size_++;

    if (IsEmpty()) {
      head_ = tail_ = elem;
      elem->prev = elem->next = nullptr;
    } else {
      // Link previous tail and new tail together.
      tail_->next = elem;
      elem->prev = tail_;

      // There is nothing on the right side of new tail.
      elem->next = nullptr;

      // Set tail point new element.
      tail_ = elem;
    }
  }

  KernelListElement<T>* GetFront() const { return head_; }
  KernelListElement<T>* GetTail() const { return tail_; }

  KernelListElement<T>* PopFront() {
    if (IsEmpty()) {
      return nullptr;
    } else {
      size_--;

      auto* head_to_return = head_;
      auto* new_head = head_->next;

      // Detach head from the list.
      head_to_return->RemoveSelfFromList();

      // If current head is the ONLY element.
      if (!new_head) {
        head_ = tail_ = nullptr;
      } else {
        head_ = new_head;
      }

      return head_to_return;
    }
  }

  KernelListElement<T>* PopBack() {
    if (IsEmpty()) {
      return nullptr;
    } else {
      size_--;

      auto* tail_to_return = tail_;
      auto* new_tail = tail_->prev;

      // Detach head from the list.
      tail_to_return->RemoveSelfFromList();

      // If current tail is the ONLY element.
      if (!new_tail) {
        head_ = tail_ = nullptr;
      } else {
        tail_ = new_tail;
      }

      return tail_to_return;
    }
  }

 private:
  KernelListElement<T>* head_;
  KernelListElement<T>* tail_;

  int size_ = 0;
};

}  // namespace Kernel
#endif
