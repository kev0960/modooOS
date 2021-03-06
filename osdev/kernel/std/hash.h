#ifndef STD_HASH_H
#define STD_HASH_H

#include "types.h"

namespace Kernel {
namespace std {

template <typename Key>
struct hash {};

template <>
struct hash<uint64_t> {
  size_t operator()(const uint64_t& xx) const {
    uint64_t x = xx;
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ull;
    x = (x ^ (x >> 27)) * 0x94d049bb133111ebull;
    x = x ^ (x >> 31);
    return x;
  }
};

template <>
struct hash<uint32_t> {
  size_t operator()(const uint32_t& x) const { return hash<uint64_t>()(x); }
};

template <>
struct hash<int> {
  size_t operator()(const int& x) const { return hash<uint64_t>()(x); }
};

template <>
struct hash<char> {
  size_t operator()(const char& x) const { return hash<uint64_t>()(x); }
};

}  // namespace std
}  // namespace Kernel
#endif
