#ifndef MEMORY_H
#define MEMORY_H

#include "kmalloc.h"
#include "types.h"
#include "utility.h"

namespace Kernel {
namespace std {

template <typename T>
class allocator {
 public:
  using pointer = T*;

  // Allocate n bytes.
  constexpr T* allocate(size_t n) { return kmalloc(n); }
  constexpr void deallocate(T* p, size_t n) { kfree(p); }
};

template <typename Alloc>
class allocator_traits {
  using pointer = typename Alloc::pointer;

  [[nodiscard]] static pointer allocate(Alloc& a, size_t n) { a.allocate(n); }
  static void deallocate(Alloc& a, pointer p, size_t n) { a.deallocate(p, n); }
  template <typename T, class... Args>
  static void construct(Alloc& a, T* p, Args&&... args) {
    // placement new.
    new (p) T(std::forward<Args>(args)...);
  }
  template <typename T>
  static void destroy(Alloc& a, T* p) {
    p->~T();
  }
};

}  // namespace std
}  // namespace Kernel

#endif
