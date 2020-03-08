#ifndef ARRAY_H
#define ARRAY_H

#include "string.h"
#include "types.h"

namespace Kernel {
namespace std {

template <typename T, size_t Size>
class array {
 public:
  const T& at(size_t i) const { return data_[i]; }
  T& operator[](size_t i) { return data_[i]; }
  constexpr size_t size() const { return Size; }
  T* data() { return data_; }
  void fill(const T& value) {
    for (size_t i = 0; i < Size; i++) {
      data_[i] = value;
    }
  }

  template <typename U, size_t SrcSize>
  void CopyFrom(std::array<U, SrcSize>& src) {
    static_assert(sizeof(T) * Size == sizeof(U) * SrcSize);
    memcpy(data_, &src[0], sizeof(T) * Size);
  }

 private:
  T data_[Size];
};

}  // namespace std
}  // namespace Kernel
#endif
