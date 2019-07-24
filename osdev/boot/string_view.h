#ifndef STRING_VIEW_H
#define STRING_VIEW_H

#include "string_util.h"
#include "types.h"

namespace Kernel {
template <typename CharT>
class basic_string_view {
 public:
  basic_string_view(const CharT* s) : str_(s) { size_ = strlen(s); }

  constexpr size_t size() const { return size_; }
  CharT operator[](size_t i) const { return str_[i]; }

 private:
  size_t size_;
  const CharT* str_;
};

using string_view = basic_string_view<char>;
}  // namespace Kernel
#endif
