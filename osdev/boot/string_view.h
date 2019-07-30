#ifndef STRING_VIEW_H
#define STRING_VIEW_H

#include "string_util.h"
#include "types.h"

namespace Kernel {
template <typename CharT>
class basic_string_view {
 public:
  basic_string_view(const CharT* s) : str_(s) { size_ = strlen(s); }
  basic_string_view(const CharT* s, size_t count) : str_(s), size_(count) {}

  constexpr size_t size() const { return size_; }
  CharT operator[](size_t i) const { return str_[i]; }

  constexpr basic_string_view substr(size_t pos = 0,
                                     size_t count = npos) const {
    if (count == npos) {
      return basic_string_view(str_ + pos, size_ - pos);
    } else if (count + pos >= size_) {
      return basic_string_view(str_ + pos, size_ - pos);
    } else {
      return basic_string_view(str_ + pos, count);
    }
  }

  constexpr void remove_prefix(size_t n) {
    str_ += n;
    size_ -= n;
  }

  constexpr bool empty() const { return size_ == 0; }

  constexpr size_t find_first_of(CharT c, size_t pos, size_t count) const
      noexcept {
    size_t i = pos;
    for (; i < min(size_, pos + count); i++) {
      if (str_[i] == c) {
        return i;
      }
    }
    return npos;
  }

 private:
  const CharT* str_;
  size_t size_;
};

using string_view = basic_string_view<char>;
}  // namespace Kernel
#endif
