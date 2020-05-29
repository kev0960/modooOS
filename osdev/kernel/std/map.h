#ifndef MAP_H
#define MAP_H

#include "set.h"

namespace Kernel {
namespace std {

template <typename Key, typename Value>
struct KeyVal {
  Key first;
  Value second;

  bool operator<(const KeyVal& kv) const { return first < kv.first; }
  bool operator==(const KeyVal& kv) const { return first == kv.first; }
  bool operator>(const KeyVal& kv) const { return first > kv.first; }

  KeyVal(const Key& key) : first(key) {}
  KeyVal(const Key& key, const Value& val) : first(key), second(val) {}
  KeyVal(Key&& key, Value&& val)
      : first(std::move(key)), second(std::move(val)) {}
};

template <typename Key, typename Value>
class MapIterator {
 public:
  using value_type = KeyVal<Key, Value>;
  value_type operator*() { return *iter; }
  const value_type operator*() const { return *iter; }

  MapIterator(const typename set<KeyVal<Key, Value>>::iterator& itr)
      : iter(itr) {}
  MapIterator(typename set<KeyVal<Key, Value>>::iterator&& itr)
      : iter(std::move(itr)) {}

  MapIterator(const MapIterator& it) : iter(it.iter) {}
  MapIterator(MapIterator&& it) : iter(std::move(it.iter)) {}
  MapIterator& operator=(const MapIterator& it) {
    iter = it.iter;
    return *this;
  }

  MapIterator& operator++() {
    ++iter;
    return *this;
  }

  value_type* operator->() { return &(*iter); }

  bool operator==(const MapIterator& it) const { return iter == it.iter; }
  bool operator!=(const MapIterator& it) const { return iter != it.iter; }

  typename set<KeyVal<Key, Value>>::iterator& GetSetIter() { return iter; }

 private:
  typename set<KeyVal<Key, Value>>::iterator iter;
};

template <typename Key, typename Value,
          typename Allocator =
              std::allocator<BinaryTreeNode<KeyVal<const Key, Value>>>>
class map {
 public:
  using iterator = MapIterator<const Key, Value>;
  using value_type = pair<const Key, Value>;

  map() = default;

  // Dump ways to copy
  map(const map& m) {
    for (auto itr = m.begin(); itr != m.end(); ++itr) {
      operator[]((*itr).first) = (*itr).second;
    }
  }

  map& operator=(const map& m) {
    for (auto itr = m.begin(); itr != m.end(); ++itr) {
      operator[]((*itr).first) = (*itr).second;
    }
    return *this;
  }

  Value& at(const Key& key) {
    iterator itr(key_val_.find(KeyVal<const Key, Value>(key)));
    ASSERT(itr != end());
    return itr->second;
  }

  const Value& at(const Key& key) const {
    iterator itr(key_val_.find(KeyVal<const Key, Value>(key)));
    ASSERT(itr != end());
    return itr->val;
  }

  Value& operator[](const Key& key) {
    iterator itr = find(key);
    if (itr == end()) {
      insert({key, Value()});
    }
    return at(key);
  }

  void insert(const value_type& value) {
    key_val_.insert(KeyVal<const Key, Value>(value.first, value.second));
  }

  void erase(iterator pos) { key_val_.erase(pos.GetSetIter()); }
  size_t erase(const Key& key) { return key_val_.erase(key); }

  iterator find(const Key& key) const {
    return key_val_.find(KeyVal<const Key, Value>(key));
  }

  iterator begin() const { return key_val_.begin(); }
  iterator end() const { return key_val_.end(); }

  size_t size() const { return key_val_.size(); }

  void Print() { key_val_.Print(); }

 private:
  std::set<KeyVal<const Key, Value>> key_val_;
};

}  // namespace std
}  // namespace Kernel

#endif
