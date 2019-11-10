#ifndef ARRAY_H
#define ARRAY_H

#include "types.h"

namespace Kernel {
namespace std {

template <typename T, size_t SIZE>
class array {
 public:
  const T& at(size_t i) { return data[i]; }
  T& operator[](size_t i) { return data[i]; }
  constexpr size_t size() const { return SIZE; }
  void fill(const T& value) {
    for (int i = 0; i < SIZE; i++) {
      data[i] = value;
    }
  }

 private:
  T data[SIZE];
};

}  // namespace std
}  // namespace Kernel
#endif
