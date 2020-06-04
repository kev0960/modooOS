#ifndef STRING_H
#define STRING_H

#include "string_view.h"
#include "types.h"
#include "vector.h"

namespace Kernel {

void* memcpy(void* dest, void* src, size_t count);
void* memset(void* dest, int ch, size_t count);

int strncmp(const char* lhs, const char* rhs, size_t count);

// Very simple vector based kernel string. This is not efficient but very easy
// to implement.
template <typename CharT>
class KernelBasicString {
 public:
  KernelBasicString() { str_.push_back(0); }

  KernelBasicString(const CharT* str) {
    size_t sz = 0;
    while (str[sz]) {
      sz++;
    }

    str_.reserve(sz + 1);

    for (size_t i = 0; i <= sz; i++) {
      str_.push_back(str[i]);
    }
  }

  KernelBasicString(const CharT* str, size_t sz) {
    str_.reserve(sz + 1);

    for (size_t i = 0; i < sz; i++) {
      str_.push_back(str[i]);
    }

    str_.push_back(0);
  }

  KernelBasicString(const std::basic_string_view<CharT>& view) {
    str_.reserve(view.size());

    for (size_t i = 0; i < view.size(); i++) {
      str_.push_back(view[i]);
    }

    if (view.size() == 0 || back() != 0) {
      str_.push_back(0);
    }
  }

  KernelBasicString(const KernelBasicString& s) { str_ = s.str_; }
  KernelBasicString(KernelBasicString&& s) : str_(std::move(s.str_)) {}

  KernelBasicString& operator=(const KernelBasicString& s) {
    str_ = s.str_;
    return *this;
  }
  KernelBasicString& operator=(KernelBasicString&& s) {
    str_ = std::move(s.str_);
    return *this;
  }

  // Remove last null.
  size_t size() const { return str_.size() - 1; }
  size_t capacity() const { return str_.capacity(); }

  CharT& operator[](size_t index) { return str_[index]; }
  const CharT& at(size_t index) const { return str_.at(index); }

  CharT& back() { return str_[size()]; }

  const char* c_str() const { return &str_.at(0); }

  KernelBasicString substr(size_t start, size_t len = npos) const {
    if (len == npos) {
      len = size() - start;
    }

    return KernelBasicString(&str_.at(start), len);
  }

  size_t find(CharT c, size_t from = 0) const {
    for (size_t i = from; i < size(); i++) {
      if (str_.at(i) == c) {
        return i;
      }
    }
    return npos;
  }

  bool operator==(const KernelBasicString& s) const {
    if (size() != s.size()) {
      return false;
    }
    for (size_t i = 0; i < size(); i++) {
      if (at(i) != s.at(i)) {
        return false;
      }
    }
    return true;
  }

  bool operator!=(const KernelBasicString& s) const { return !(operator==(s)); }

  bool operator==(std::basic_string_view<CharT> s) const {
    if (size() != s.size()) {
      return false;
    }
    for (size_t i = 0; i < size(); i++) {
      if (at(i) != s[i]) {
        return false;
      }
    }
    return true;
  }

  bool operator!=(std::basic_string_view<CharT> s) const {
    return !(operator==(s));
  }

 private:
  std::vector<CharT> str_;
};

using KernelString = KernelBasicString<char>;

std::vector<KernelString> Split(const KernelString& ks, char delim);

}  // namespace Kernel

#endif
