#ifndef MEMORY_H
#define MEMORY_H

#include "cpp_macro.h"
#include "../kernel/kmalloc.h"
#include "types.h"
#include "utility.h"

void* operator new(Kernel::size_t, void* p);

namespace Kernel {
namespace std {

template <typename T>
class allocator {
 public:
  using pointer = T*;

  // Allocate n bytes.
  constexpr T* allocate(size_t n) { return reinterpret_cast<T*>(kmalloc(n)); }
  constexpr void deallocate(T* p, size_t n) {
    UNUSED(n);
    kfree(p);
  }
};

template <typename Alloc>
class allocator_traits {
 public:
  using pointer = typename Alloc::pointer;

  [[nodiscard]] static pointer allocate(Alloc& a, size_t n) {
    return a.allocate(n);
  }
  static void deallocate(Alloc& a, pointer p, size_t n) { a.deallocate(p, n); }
  template <typename T, class... Args>
  static void construct(Alloc& a, T* p, Args&&... args) {
    UNUSED(a);

    // placement new.
    ::new (static_cast<void*>(p)) T(std::forward<Args>(args)...);
  }
  template <typename T>
  static void destroy(Alloc& a, T* p) {
    UNUSED(a);
    p->~T();
  }
};


}  // namespace std
}  // namespace Kernel

#endif
