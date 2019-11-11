#ifndef VECTOR_H
#define VECTOR_H

#include "memory.h"
#include "printf.h"
#include "utility.h"

namespace Kernel {
namespace std {

template <typename T, typename Allocator = std::allocator<T>>
class vector {
 public:
  vector() : size_(0) {
    alloc_size_ = 1;
    data_ = alloc_::allocate(allocator_, sizeof(T));
  }

  vector(size_t size) : size_(size), alloc_size_(size_) {
    data_ = alloc_::allocate(allocator_, sizeof(T) * size);
    for (size_t i = 0; i < size_; i++) {
      alloc_::construct(allocator_, &data_[i]);
    }
  }

  T& operator[](size_t index) { return data_[index]; }
  const T& at(size_t index) const { return data_[index]; }
  size_t size() const { return size_; }
  size_t capacity() const { return alloc_size_; }

  void push_back(const T& t) {
    if (size_ >= alloc_size_) {
      ExpandAlloc();
    }

    // Now construct new item at size_.
    alloc_::construct(allocator_, &data_[size_], t);
    size_++;
  }

  void push_back(T&& t) {
    if (size_ >= alloc_size_) {
      ExpandAlloc();
    }

    // Now construct new item at size_.
    alloc_::construct(allocator_, &data_[size_], std::move(t));
    size_++;
  }

  void pop_back() {
    alloc_::destroy(allocator_, &data_[size_ - 1]);
    size_--;
  }

  ~vector() {
    for (size_t i = 0; i < size_; i++) {
      alloc_::destroy(allocator_, &data_[i]);
    }
    alloc_::deallocate(allocator_, data_, sizeof(T) * alloc_size_);
  }

 private:
  size_t size_;
  size_t alloc_size_;

  T* data_;
  Allocator allocator_;

  using alloc_ = std::allocator_traits<Allocator>;

  // Double the size of allocated memory.
  void ExpandAlloc() {
    size_t new_alloc_size = alloc_size_ * 2;
    T* new_loc = alloc_::allocate(allocator_, sizeof(T) * new_alloc_size);

    // Move previously allocated elements to the new memory.
    for (size_t i = 0; i < size_; i++) {
      alloc_::construct(allocator_, &new_loc[i], std::move(data_[i]));
    }

    // Destruct elements at previous location.
    for (size_t i = 0; i < size_; i++) {
      alloc_::destroy(allocator_, &data_[i]);
    }
    alloc_::deallocate(allocator_, data_, sizeof(T) * alloc_size_);

    alloc_size_ = new_alloc_size;
    data_ = new_loc;
  }
};

}  // namespace std
}  // namespace Kernel
#endif
