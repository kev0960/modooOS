#ifndef ARRAY_H
#define ARRAY_H

#include "string.h"
#include "types.h"

namespace Kernel {
namespace std {

template <typename T, size_t Size>
class array {
 public:
  const T& at(size_t i) const { return data[i]; }
  T& operator[](size_t i) { return data[i]; }
  constexpr size_t size() const { return Size; }
  void fill(const T& value) {
    for (size_t i = 0; i < Size; i++) {
      data[i] = value;
    }
  }

  template <typename U, size_t SrcSize>
  void CopyFrom(std::array<U, SrcSize>& src) {
    static_assert(sizeof(T) * Size == sizeof(U) * SrcSize);
    memcpy(data, &src[0], sizeof(T) * Size);
  }

 private:
  T data[Size];
};

}  // namespace std
}  // namespace Kernel
#endif
