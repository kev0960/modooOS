#ifndef STD_HASH_SET
#define STD_HASH_SET

#include "../kernel/qemu_log.h"
#include "hash.h"
#include "types.h"
#include "vector.h"

namespace Kernel {
namespace std {

template <typename Key>
struct HashSetSlot {
  bool is_tomb = false;
  Key* key = nullptr;
};

template <typename Key>
class HashSetInternal {
 public:
  constexpr static size_t kInitNumSlots = 1024;

  // Load factor = 1 / 2
  constexpr static size_t kInverseLoadFactor = 2;

  HashSetInternal() : slots_(kInitNumSlots), num_elements_(0) {}

  void InsertKey(Key* key) {
    RehashIfLoaded();
    InsertKeyInternal(key, slots_);
  }

  // If found, returns the Key in the slot. Otherwise, returns nullptr.
  Key* SearchKey(const Key* key) const {
    size_t index = std::hash<Key>()(*key) % slots_.size();

    // Start probing.
    while (true) {
      if (slots_.at(index).key != nullptr) {
        if ((*slots_.at(index).key) == *key) {
          return slots_.at(index).key;
        }
      } else if (slots_.at(index).key == nullptr && !slots_.at(index).is_tomb) {
        return nullptr;
      }

      index++;

      // Make sure it wraps around.
      if (index >= slots_.size()) {
        index = 0;
      }
    }
  }

  // Remove the key from the set.
  Key* RemoveKey(const Key* key) {
    size_t index = std::hash<Key>()(*key) % slots_.size();

    // Start probing.
    while (true) {
      if (slots_[index].key != nullptr) {
        if ((*slots_[index].key) == *key) {
          Key* found = slots_[index].key;
          slots_[index].key = nullptr;
          slots_[index].is_tomb = true;

          num_elements_--;
          return found;
        }
      } else if (slots_[index].key == nullptr && !slots_[index].is_tomb) {
        return nullptr;
      }

      index++;

      // Make sure it wraps around.
      if (index >= slots_.size()) {
        index = 0;
      }
    }
  }

  void RehashIfLoaded() {
    if (num_elements_ * kInverseLoadFactor < slots_.size()) {
      return;
    }

    vector<HashSetSlot<Key>> new_slots(slots_.size() * 2);
    for (size_t i = 0; i < slots_.size(); i++) {
      Key* key = slots_[i].key;
      if (key != nullptr) {
        InsertKeyInternal(key, new_slots);
      }
    }

    slots_ = std::move(new_slots);
  }

 private:
  void InsertKeyInternal(Key* key, std::vector<HashSetSlot<Key>>& slots) {
    size_t index = std::hash<Key>()(*key) % slots.size();

    // Start probing.
    while (true) {
      if (slots[index].key == nullptr) {
        slots[index].key = key;
        slots[index].is_tomb = false;

        num_elements_++;
        break;
      }

      index++;

      // Make sure it wraps around.
      if (index >= slots.size()) {
        index = 0;
      }
    }
  }

  vector<HashSetSlot<Key>> slots_;
  size_t num_elements_;
};

template <typename Key>
class HashSet {
 public:
  using value_type = Key;

  void insert(const value_type& v) { hash_set_.InsertKey(new Key(v)); }
  size_t erase(const value_type& v) {
    Key* key = hash_set_.RemoveKey(&v);
    if (key != nullptr) {
      delete key;
      return 1;
    }
    return 0;
  }

  // Returns nullptr if Key is not found.
  const Key* find(const value_type& v) const { return hash_set_.SearchKey(&v); }

 private:
  HashSetInternal<Key> hash_set_;
};

}  // namespace std
}  // namespace Kernel
#endif
