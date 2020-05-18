#ifndef KERNEL_LIST_H
#define KERNEL_LIST_H

#include "../std/iterator.h"
#include "../std/types.h"
#include "kernel_util.h"

namespace Kernel {

// List that does not require dynamic allocation.
template <typename T>
class KernelList;

template <typename T>
class KernelListElement {
 public:
  KernelListElement(KernelList<T>* stack_list)
      : prev(nullptr), next(nullptr), stack_list_(stack_list) {}

  void PushFront() { stack_list_->push_front(this); }
  void PushBack() { stack_list_->push_back(this); }
  void Detach() {
    if (prev) {
      prev->next = next;
    }
    if (next) {
      next->prev = prev;
    }

    prev = next = nullptr;
  }

  // Remove self from the list.
  void RemoveSelfFromList() {
    stack_list_->remove(this);
    Detach();
  }

  void ChangeList(KernelList<T>* stack_list) {
    // Make sure that it is detached.
    if (prev != nullptr || next != nullptr) {
      return;
    }
    stack_list_ = stack_list;
  }

  T& Get() { return t; }

  void Set(const T& data) { t = data; }

  KernelListElement* prev;
  KernelListElement* next;

 private:
  T t;

  KernelList<T>* stack_list_;
};

template <typename T>
class KernelListIterator {
 public:
  using iterator_category = std::bidirectional_iterator_tag;
  using value_type = T;
  using pointer = T*;
  using reference = T&;
  using difference_type = void;

  explicit KernelListIterator(KernelListElement<T>* elem) : current_(elem) {}
  KernelListIterator(const KernelListIterator& itr) : current_(itr.current_) {}

  KernelListIterator& operator++() {
    if (current_ != nullptr) {
      current_ = current_->next;
    }
    return *this;
  }

  KernelListIterator operator++(int) {
    KernelListIterator iter = *this;
    if (current_ != nullptr) {
      current_ = current_->next;
    }
    return iter;
  }

  KernelListIterator& operator--() {
    if (current_ != nullptr) {
      current_ = current_->prev;
    }
    return *this;
  }

  KernelListIterator operator--(int) {
    KernelListIterator iter = *this;
    if (current_ != nullptr) {
      current_ = current_->prev;
    }
    return iter;
  }

  KernelListIterator operator=(const KernelListIterator& itr) {
    current_ = itr.current_;
  }

  bool operator==(KernelListIterator iter) { return current_ == iter.current_; }
  bool operator!=(KernelListIterator iter) { return current_ != iter.current_; }
  reference operator*() { return current_->Get(); }

 private:
  KernelListElement<T>* current_;
};

template <typename T>
class KernelList {
 public:
  KernelList() : head_(nullptr), tail_(nullptr) {}

  // Copy construct is not available.
  KernelList(const KernelList&) = delete;
  KernelList(KernelList&& list) {
    head_ = list.head_;
    tail_ = list.tail_;
    size_ = list.size_;

    list.head_ = nullptr;
    list.tail_ = nullptr;
    list.size_ = 0;
  }

  bool empty() const { return head_ == nullptr; }
  size_t size() const { return size_; }

  void push_front(KernelListElement<T>* elem) {
    size_++;

    if (head_ == elem || tail_ == elem) {
      PANIC();
    }
    // If empty.
    if (empty()) {
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

  void push_back(KernelListElement<T>* elem) {
    size_++;

    if (head_ == elem || tail_ == elem) {
      PANIC();
    }
    if (empty()) {
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

  KernelListElement<T>* front() const { return head_; }
  KernelListElement<T>* back() const { return tail_; }

  KernelListElement<T>* pop_front() {
    if (empty() && size_ != 0) {
      PANIC();
    }
    if (empty()) {
      return nullptr;
    } else {
      size_--;

      auto* head_to_return = head_;
      auto* new_head = head_->next;

      // Detach head from the list.
      head_to_return->Detach();

      // If current head is the ONLY element.
      if (!new_head) {
        head_ = tail_ = nullptr;
      } else {
        head_ = new_head;
      }

      return head_to_return;
    }
  }

  KernelListElement<T>* pop_back() {
    if (empty() && size_ != 0) {
      PANIC();
    }
    if (empty()) {
      return nullptr;
    } else {
      size_--;

      auto* tail_to_return = tail_;
      auto* new_tail = tail_->prev;

      // Detach head from the list.
      tail_to_return->Detach();

      // If current tail is the ONLY element.
      if (!new_tail) {
        head_ = tail_ = nullptr;
      } else {
        tail_ = new_tail;
      }

      return tail_to_return;
    }
  }

  // Do not call this directly.
  void remove(KernelListElement<T>* elem) {
    if (head_ == elem) {
      head_ = elem->next;
    }
    if (tail_ == elem) {
      tail_ = elem->prev;
    }
    size_--;
  }

  KernelListIterator<T> begin() const { return KernelListIterator<T>(head_); }
  KernelListIterator<T> end() const { return KernelListIterator<T>(nullptr); }

// private:
  KernelListElement<T>* head_;
  KernelListElement<T>* tail_;

  int size_ = 0;
};
}  // namespace Kernel

#endif
