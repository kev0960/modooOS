#ifndef STRING_VIEW_H
#define STRING_VIEW_H

#include "string_util.h"
#include "types.h"
#include "vector.h"

namespace Kernel {
namespace std {

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
    size_ -= min(n, size_);
  }

  constexpr bool empty() const { return size_ == 0; }

  constexpr size_t find_first_of(CharT c, size_t pos) const noexcept {
    size_t i = pos;
    for (; i < size_; i++) {
      if (str_[i] == c) {
        return i;
      }
    }
    return npos;
  }

  constexpr size_t find_last_of(CharT c) const noexcept {
    size_t i = size_ - 1;
    while (true) {
      if (str_[i] == c) {
        return i;
      } else if (i == 0) {
        break;
      }
      i--;
    }
    return npos;
  }

  constexpr size_t find_first_of(CharT c, size_t pos,
                                 size_t count) const noexcept {
    size_t i = pos;
    for (; i < min(size_, pos + count); i++) {
      if (str_[i] == c) {
        return i;
      }
    }
    return npos;
  }

  constexpr bool operator==(basic_string_view str) const {
    if (size_ != str.size_) {
      return false;
    }
    for (size_t i = 0; i < size_; i++) {
      if (str_[i] != str[i]) {
        return false;
      }
    }
    return true;
  }

  constexpr bool operator!=(basic_string_view str) const {
    return !(operator==(str));
  }

 private:
  const CharT* str_;
  size_t size_;
};

using string_view = basic_string_view<char>;

template <typename CharT>
std::vector<basic_string_view<CharT>> Split(basic_string_view<CharT> s,
                                            char delim) {
  std::vector<basic_string_view<CharT>> sp;

  size_t current = 0;
  while (current < s.size()) {
    size_t next = s.find_first_of(delim, current);
    if (next == npos) {
      sp.push_back(s.substr(current));
      break;
    } else if (next != current) {
      sp.push_back(s.substr(current, next - current));
    }

    // Note that we ignore consecutive delimiters.

    current = next + 1;
  }

  return sp;
}

}  // namespace std
}  // namespace Kernel
#endif
