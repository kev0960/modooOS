#ifndef STD_HASH_MAP
#define STD_HASH_MAP

#include "hash_set.h"

namespace Kernel {
namespace std {

template <typename Key, typename Value>
struct KeyValHashMap {
  Key key;
  Value value;

  KeyValHashMap() {}
  KeyValHashMap(const Key& k) : key(k) {}
  KeyValHashMap(const Key& k, const Value& v) : key(k), value(v) {}

  bool operator==(const KeyValHashMap& kv) { return key == kv.key; }
};

template <typename Key, typename Value>
struct hash<KeyValHashMap<Key, Value>> {
  size_t operator()(const KeyValHashMap<Key, Value>& kv) const {
    return std::hash<Key>()(kv.key);
  }
};

template <typename Key, typename Value>
class HashMap {
 public:
  void insert(const Key& key, const Value& val) {
    hash_set_.insert(KeyValHashMap<Key, Value>(key, val));
  }

  void insert(const Key& key) {
    hash_set_.insert(KeyValHashMap<Key, Value>(key));
  }

  size_t erase(const Key& key) {
    KeyValHashMap<Key, Value> kv;
    kv.key = key;

    return hash_set_.erase(kv);
  }

  const Value* find(const Key& key) const {
    KeyValHashMap<Key, Value> kv;
    kv.key = key;

    auto* key_val = hash_set_.find(kv);
    if (key_val == nullptr) {
      return nullptr;
    }

    return &key_val->value;
  }

  Value& operator[](const Key& key) {
    KeyValHashMap<Key, Value> kv;
    kv.key = key;

    auto* key_val = hash_set_.find(kv);
    if (key_val == nullptr) {
      insert(key);
    }

    key_val = hash_set_.find(kv);
    return key_val->value;
  }

 private:
  HashSet<KeyValHashMap<Key, Value>> hash_set_;
};

}  // namespace std
}  // namespace Kernel
#endif
