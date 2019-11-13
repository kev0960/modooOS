#ifndef VECTOR_H
#define VECTOR_H

#include "iterator.h"
#include "memory.h"
#include "utility.h"

namespace Kernel {
namespace std {

template <typename T>
class VectorIterator {
 public:
  using iterator_category = std::random_access_iterator_tag;
  using value_type = T;
  using pointer = T*;
  using reference = T&;
  using difference_type = int;

  explicit VectorIterator(T* elem) : current_(elem) {}
  VectorIterator(const VectorIterator& itr) : current_(itr.current_) {}

  VectorIterator& operator++() {
    current_++;
    return *this;
  }

  VectorIterator operator++(int) {
    VectorIterator iter = *this;
    current_++;
    return iter;
  }

  VectorIterator& operator+=(int n) {
    current_ += n;
    return *this;
  }

  VectorIterator& operator--() {
    current_--;
    return *this;
  }

  VectorIterator operator--(int) {
    VectorIterator iter = *this;
    current_--;
    return iter;
  }

  VectorIterator& operator-=(int n) {
    current_ -= n;
    return *this;
  }

  VectorIterator operator=(const VectorIterator& itr) {
    current_ = itr.current_;
  }

  bool operator==(VectorIterator iter) { return current_ == iter.current_; }
  bool operator!=(VectorIterator iter) { return current_ != iter.current_; }
  reference operator*() { return *current_; }

 private:
  T* current_;
};

template <typename T, typename Allocator = std::allocator<T>>
class vector {
 public:
  using iterator = VectorIterator<T>;

  vector() : size_(0), alloc_size_(0), data_(nullptr) {}

  vector(size_t size) : size_(size), alloc_size_(size_) {
    data_ = alloc_::allocate(allocator_, sizeof(T) * size);
    for (size_t i = 0; i < size_; i++) {
      alloc_::construct(allocator_, &data_[i]);
    }
  }

  vector(const vector& v) { CopyFrom(v); }

  vector(vector&& v) {
    size_ = v.size_;
    alloc_size_ = v.alloc_size_;
    data_ = v.data_;

    v.size_ = 0;
    v.alloc_size_ = 0;
    v.data_ = nullptr;
  }

  vector& operator=(const vector& v) {
    // If current allocated memory cannot handle v's elements, we should
    // deallocate and reallocate.
    if (alloc_size_ < v.size_) {
      Destroy();

      // Copy fresh.
      CopyFrom(v);
    } else {
      // Otherwise we should just destroy elements.
      for (size_t i = 0; i < size_; i++) {
        alloc_::destroy(allocator_, &data_[i]);
      }

      size_ = v.size_;

      for (size_t i = 0; i < v.size_; i++) {
        // Copy construct.
        alloc_::construct(allocator_, &data_[i], v.data_[i]);
      }
    }

    return *this;
  }

  vector& operator=(vector&& v) {
    size_ = v.size_;
    alloc_size_ = v.alloc_size_;
    data_ = v.data_;

    v.size_ = 0;
    v.alloc_size_ = 0;
    v.data_ = nullptr;

    return *this;
  }

  T& operator[](size_t index) { return data_[index]; }
  const T& at(size_t index) const { return data_[index]; }
  size_t size() const { return size_; }
  size_t capacity() const { return alloc_size_; }

  void reserve(size_t sz) {
    if (sz < alloc_size_) {
      return;
    }

    Resize(sz);
  }

  void push_back(const T& t) {
    if (size_ >= alloc_size_) {
      Resize(GetNewAllocSize());
    }

    // Now construct new item at size_.
    alloc_::construct(allocator_, &data_[size_], t);
    size_++;
  }

  void push_back(T&& t) {
    if (size_ >= alloc_size_) {
      Resize(GetNewAllocSize());
    }

    // Now construct new item at size_.
    alloc_::construct(allocator_, &data_[size_], std::move(t));
    size_++;
  }

  void pop_back() {
    alloc_::destroy(allocator_, &data_[size_ - 1]);
    size_--;
  }

  iterator begin() { return iterator(data_); }
  iterator end() { return iterator(data_ + size_); }

  ~vector() { Destroy(); }

 private:
  size_t size_;
  size_t alloc_size_;

  T* data_;
  Allocator allocator_;

  using alloc_ = std::allocator_traits<Allocator>;

  // Double the size of allocated memory.
  void Resize(size_t new_alloc_size) {
    T* new_loc = alloc_::allocate(allocator_, sizeof(T) * new_alloc_size);

    // Move previously allocated elements to the new memory.
    for (size_t i = 0; i < size_; i++) {
      alloc_::construct(allocator_, &new_loc[i], std::move(data_[i]));
    }

    // Destruct elements at previous location.
    for (size_t i = 0; i < size_; i++) {
      alloc_::destroy(allocator_, &data_[i]);
    }
    if (data_ != nullptr) {
      alloc_::deallocate(allocator_, data_, sizeof(T) * alloc_size_);
    }

    alloc_size_ = new_alloc_size;
    data_ = new_loc;
  }

  void Destroy() {
    if (!data_) {
      return;
    }

    for (size_t i = 0; i < size_; i++) {
      alloc_::destroy(allocator_, &data_[i]);
    }
    alloc_::deallocate(allocator_, data_, sizeof(T) * alloc_size_);
  }

  void CopyFrom(const vector& v) {
    size_ = v.size_;
    alloc_size_ = v.alloc_size_;

    if (alloc_size_ == 0) {
      return;
    }

    data_ = alloc_::allocate(allocator_, sizeof(T) * alloc_size_);
    for (size_t i = 0; i < size_; i++) {
      // Copy construct.
      alloc_::construct(allocator_, &data_[i], v.data_[i]);
    }
  }

  size_t GetNewAllocSize() {
    if (alloc_size_ == 0) {
      return 1;
    } else {
      return alloc_size_ * 2;
    }
  }
};

}  // namespace std
}  // namespace Kernel
#endif
