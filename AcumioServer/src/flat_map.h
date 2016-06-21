#ifndef AcumioServer_flat_map_h
#define AcumioServer_flat_map_h
//============================================================================
// Name        : flat_map.h
// Author      : Bill Province (bill@acumio.com)
// Version     :
// Copyright   : Copyright (C) 2016 Acumio
// Description : A flat array of string-based keys with associated uint32_t
//               values. Ordering of the keys is maintained via insertion
//               sort.
//
//               While insertion sort has limited scalability, for small
//               enough datasets, it will be effective due to the locality of
//               the data.
//
//               This structure comes with some built-in boundaries that
//               prevent growth beyond a fixed size. If adding a particular
//               data element would cause the map to exceed the size limits,
//               we reject the operation.
//
//               The strings are tracked via a StringAllocator, and thus
//               the keys are actually integer offsets into a string array.
//               The values are typed as uint32_t, but are tracked with an
//               ObjectAllocator.
//============================================================================

#include <memory>
#include <stack>
#include <string.h>
#include "object_allocator.h"
#include "string_allocator.h"

namespace acumio {
namespace collection {

template <typename EltType>
class FlatMap {
 public:
  struct IteratorElement {
    IteratorElement(const char* k, uint32_t v) : key(k), value(v) {}
    IteratorElement() : IteratorElement(nullptr, UINT32_C(0)) {}
    const char* key;
    uint32_t value;
  };

  class Iterator  :
      public std::iterator<std::bidirectional_iterator_tag, IteratorElement> {
   public:
    Iterator(const FlatMap* container, uint8_t node_id) : container_(container),
        node_id_(node_id), saved_tmp_(nullptr), saved_val_(nullptr) {}

    // Note that Iterator is unusable with null container.
    Iterator() : Iterator(nullptr, 0) {}

    Iterator(const Iterator& copy) :
        Iterator(copy.container_, copy.node_id_) {}

    ~Iterator() {}

    // pre-increment/decrement
    Iterator& operator++() {
      node_id_++;
      if (node_id_ > container_.size()) {
        node_id_ = container_.size();
      }
      return *this;
    }

    Iterator& operator--() {
      if (node_id_ == 0) {
        node_id_ = container_.size();
      } else {
        node_id_--;
      }
      return *this;
    }

    // post-increment/decrement
    Iterator& operator++(int) {
      saved_tmp_.reset(new Iterator(*this));
      node_id_++;
      if (node_id_ > container_.size()) {
        node_id_ = container_.size();
      }
      return *saved_tmp_;
    }

    Iterator& operator--(int) {
      saved_tmp_.reset(new Iterator(*this));
      if (node_id_ == 0) {
        node_id_ = container_.size();
      } else {
        node_id_--;
      }
      return *saved_tmp_;
    }

    bool operator==(const Iterator& other) const {
      return container_ == other.container_ && node_id_ == other.node_id_;
    }

    bool operator!=(const Iterator& other) const {
      return container_ != other.container_ || node_id_ != other.node_id_;
    }

    IteratorElement operator*() {
      IteratorElement t;
      if (node_id_ >= container_->size()) {
        t.key = nullptr;
        t.value = 0;
        return t;
      }
      t.key = container_->GetKey(node_id_);
      t.value = container_->GetValue(node_id_);
      return t;
    }

    const IteratorElement* operator->() {
      const char* key = nullptr;
      uint32_t value = 0;
      if (node_id_ < size_) {
        key = container_->GetKey(node_id_);
        value = container_->GeValue(node_id_);
      }
      saved_val_.reset(new IteratorElement(key, value));
      return saved_val_.get();
    }

   private:
    const FlatMap* container_;
    uint8_t node_id_;
    std::unique_ptr<Iterator> saved_tmp_;
    std::unique_ptr<IteratorElement> saved_val_;
  };

  // Undefined if map_size > 0 and either allocator is null.
  // This object does not take ownership of either key_allocator or
  // object allocator. The expectation is that we will use the key_allocator
  // to be shared between different versions of a FlatMap (in TxAwareFlatMap),
  // and we will use the object_allocator to be shared between all
  // FlatMaps of the given EltType.
  FlatMap(StringAllocator* key_allocator,
          ObjectAllocator<EltType>* object_allocator,
          uint8_t max_size, bool allow_dups) : key_allocator_(key_allocator),
      object_allocator_(object_allocator), max_size_(max_size), size_(0),
      allow_dups_(allow_dups), keys_(nullptr), values_(nullptr) {
    if (max_size_ > 0) {
      keys_ = new uint16_t[max_size_];
      values_ = new uint32_t[max_size_];
    }
  }

  inline FlatMap() : FlatMap(nullptr, nullptr, 0, false) {}

  FlatMap(const FlatMap& other) : key_allocator_(other.key_allocator_),
      object_allocator_(other.object_allocator_),
      max_size_(other.max_size_), size_(other.size_),
      allow_dups_(other.allow_dups_), keys_(nullptr), values_(nullptr) {
    if (max_size_ > 0) {
      keys_ = new uint16_t[max_size_];
      values_ = new uint32_t[max_size_];
      if (size_ > 0) {
        for (uint8_t i = size_ - 1; i > 0; i--) {
          key_allocator_->AddReference(keys_[i] = other.keys_[i]);
          object_allocator_->AddReference(values_[i] = other.values_[i]);
        }
        key_allocator_->AddReference(keys_[0] = other.keys_[0]);
        object_allocator_->AddReference(values_[0] = other.values_[0]);
      }
    }
  }

  // Only valid if:
  //    other.max_size_ > 0 AND
  //    if (new_key_pos == 0) then {
  //      other.size_ == 0 OR
  //      (key_allocator_->StringAt(other.keys_[0]) >
  //       key_allocator_->StringAt(key_pos))
  //    } else if new_key_pos == other.size_ then {
  //      key_allocator_->StringAt(other.keys_[other.size_ - 1]) <
  //      key_allocator_->StringAt(key_pos)
  //    } else {
  //      0 < new_key_pos < other.size_ AND
  //      key_allocator_->StringAt(other.keys_[new_key_pos]) <
  //      key_allocator_->StringAt(key_pos) AND
  //      key_allocator_->StringAt(other.keys_[new_key_pos + 1]) >
  //      key_allocator_->StringAt(key_pos)
  //    }
  //
  //    The above assumes !allow_dups, but is similar of allow_dups
  //    is true.
  FlatMap(const FlatMap& other, uint16_t key_pos, uint32_t value_pos,
      uint8_t new_elt_pos) :
      key_allocator_(other.key_allocator_),
      object_allocator_(other.object_allocator_),
      max_size_(other.max_size_), size_(other.size_ + 1),
      allow_dups_(other.allow_dups_), keys_(nullptr), values_(nullptr) {
    keys_ = new uint16_t[max_size_];
    values_ = new uint32_t[max_size_];
    for (uint8_t i = new_elt_pos - 1; i > 0; i--) {
      key_allocator_->AddReference(keys_[i] = other.keys_[i]);
      object_allocator_->AddReference(values_[i] = other.values_[i]);
    }
    key_allocator_->AddReference(keys_[new_elt_pos] = key_pos);
    object_allocator_->AddReference(values_[new_elt_pos] = value_pos);
    for (uint8_t i = new_elt_pos + 1; i < size_; i++) {
      key_allocator_->AddReference(keys_[i] = other.keys_[i-1]);
      object_allocator_->AddReference(values_[i] = other.values_[i-1]);
    }
  }

  FlatMap(const FlatMap& other, uint8_t removed_pos) :
    key_allocator_(other.key_allocator_),
    object_allocator_(other.object_allocator_),
    max_size_(other.max_size_), size_(other.size_),
    allow_dups_(other.allow_dups_), keys_(nullptr), values_(nullptr) {
    if (size_ > removed_pos) {
      size_--;
    }
    keys_ = new uint16_t[max_size_];
    values_ = new uint32_t[max_size_];
    for (uint8_t i = removed_pos - 1; i > 0; i--) {
      key_allocator_->AddReference(keys_[i] = other.keys_[i]);
      object_allocator_->AddReference(values_[i] = other.values_[i]);
    }
    for (uint8_t i = removed_pos; i < size_; i++) {
      key_allocator_->AddReference(keys_[i] = other.keys_[i+1]);
      object_allocator_->AddReference(values_[i] = other.values_[i+1]);
    }
  }

  ~FlatMap() {
    if (size_ > 0) {
      for (uint8_t i = size_ - 1; i > 0; i--) {
        key_allocator_->DropReference(keys_[i]);
        object_allocator_->DropReference(values_[i]);
      }
      key_allocator_->DropReference(keys_[0]);
      object_allocator_->DropReference(values_[0]);
    }
  }

  inline uint16_t GetIntKey(uint8_t position) const {
    return keys_[position];
  }

  inline const char* GetKey(uint8_t position) const {
    return key_allocator_->StringAt(keys_[position]);
  }

  inline const EltType& GetValue(uint8_t position) const {
    return object_allocator_->ObjectAt(values_[position]);
  }

  inline EltType& GetModifiableValue(uint8_t position) {
    return object_allocator_->ModifiableObjectAt(values_[position]);
  }

  uint8_t GetPosition(const char* key, bool* matches_key) const {
    if (size_ == 0) {
      return 0;
    }
    // first, look at final position:
    uint8_t upper_bound = size_ - 1;
    int cmp = strcmp(key, key_allocator_->StringAt(keys_[upper_bound]));
    if (cmp == 0) {
      *matches_key = true;
      return upper_bound;
    } else if (cmp > 0) {
      *matches_key = false;
      return size_;
    }
    // We now know that the key is less than the max value in the list.
    // Time to consider lower-bound.
    cmp = strcmp(key, key_allocator_->StringAt(keys_[0]));
    if (cmp == 0) {
      *matches_key = true;
      return 0;
    } else if (cmp < 0) {
      *matches_key = false;
      return 0;
    }
    // At this point, we know that the key is less than the key at the
    // upper bound (the last element in the list) and greater than the
    // key at the lower bound (0).
    uint8_t lower_bound = 0;
    uint8_t mid = (upper_bound >> 1);
    while (mid != lower_bound) {
      cmp = strcmp(key, key_allocator_->StringAt(keys_[mid]));
      if (cmp == 0) {
        *matches_key  = true;
        return mid;
      } else if (cmp < 0) {
        upper_bound = mid;
      } else {
        lower_bound = mid;
      }
      mid = ((lower_bound + upper_bound) >> 1);
    }

    *matches_key = false;
    // If we were to insert the key, it would go after all values earlier
    // that are less than the current key, and it would push up all
    // values later - including what is at the upper bound.
    return upper_bound;
  }

  // This should be invoked when allow_dups_ is true. In this case, we
  // break ties using value_pos, but still do not *truly* allow dups
  // with respect to the combination of key and value_pos.
  uint8_t GetPosition(const char* key, uint32_t value_pos,
                      bool* matches_key) const {
    if (size_ == 0) {
      return 0;
    }
    // first, look at final position:
    uint8_t upper_bound = size_ - 1;
    int cmp = strcmp(key, key_allocator_->StringAt(keys_[upper_bound]));
    if (cmp == 0) {
      if (value_pos < values_[upper_bound]) {
        cmp = -1;
      } else if (value_pos == values_[upper_bound]) {
        cmp = 0;
      } else {
        cmp = 1;
      }
    }
    if (cmp == 0) {
      *matches_key = true;
      return upper_bound;
    } else if (cmp > 0) {
      *matches_key = false;
      return size_;
    }
    // We now know that the key is less than the max value in the list.
    // Time to consider lower-bound.
    cmp = strcmp(key, key_allocator_->StringAt(keys_[0]));
    if (cmp == 0) {
      if (value_pos < values_[0]) {
        cmp = -1;
      } else if (value_pos == values_[0]) {
        cmp = 0;
      } else {
        cmp = 1;
      }
    }
    if (cmp == 0) {
      *matches_key = true;
      return 0;
    } else if (cmp < 0) {
      *matches_key = false;
      return 0;
    }
    // At this point, we know that the key is less than the key at the
    // upper bound (the last element in the list) and greater than the
    // key at the lower bound (0).
    uint8_t lower_bound = 0;
    uint8_t mid = (upper_bound >> 1);
    while (mid != lower_bound) {
      cmp = strcmp(key, key_allocator_->StringAt(keys_[mid]));
      if (cmp == 0) {
        if (value_pos < values_[mid]) {
          cmp = -1;
        } else if (value_pos == values_[mid]) {
          cmp = 0;
        } else {
          cmp = 1;
        }
      }
      if (cmp == 0) {
        *matches_key  = true;
        return mid;
      } else if (cmp < 0) {
        upper_bound = mid;
      } else {
        lower_bound = mid;
      }
      mid = ((lower_bound + upper_bound) >> 1);
    }

    *matches_key = false;
    // If we were to insert the key, it would go after all values earlier
    // that are less than the current key, and it would push up all
    // values later - including what is at the upper bound.
    return upper_bound;
  }

  // Returns nullptr if key does not match an existing entry.
  // Otherwise, returns pointer to unmodifiable value. If there
  // is more than one possible key that matches (such as if we allow dups),
  // this will return one of the entries, but no guarantee which one.
  const EltType* Get(const char* key) const {
    bool found = false;
    uint8_t pos = GetPosition(key, &found);
    if (!found) {
      return nullptr;
    }
    return &(GetValue(pos));
  }

  // Returns nullptr if key does not match an existing entry.
  // Otherwise, returns pointer to unmodifiable value.
  EltType* GetModifiable(const char* key) {
    bool found = false;
    uint8_t pos = GetPosition(key, &found);
    if (!found) {
      return nullptr;
    }
    return &(GetModifiableValue(pos));
  }

  uint16_t GetKeyPosition(const char* key, bool* exists) const {
    uint8_t pos = GetPosition(key, exists);
    if (!exists) {
      return key_allocator_->max_size();
    }
    return keys_[pos];
  }

  uint32_t GetValuePosition(const char* key, bool* exists) const {
    uint8_t pos = GetPosition(key, exists);
    if (!exists) {
      return object_allocator_->ImpossiblePosition();
    }
    return values_[pos];
  }
 
  // Returns either the position where the element was added, or max_size_
  // if the key/value could not be added. One possible reason it might not
  // be something we could add is due to duplicate keys. Another reason is just
  // that we have already reached max size. Finally, we could have an issue
  // adding the key to the key_allocator.
  uint8_t Add(const char* key, uint32_t value_position) {
    if (size_ == max_size_) {
      return max_size_;
    }

    bool exists = false;
    uint8_t pos = (allow_dups_ ? GetPosition(key, value_position, &exists)
                               : GetPosition(key, &exists));
    if (!exists) {
      return max_size_;
    }
    uint16_t key_position = key_allocator_->Add(key);
    if (key_position == key_allocator_->max_size()) {
      return max_size_;
    }
    for (uint8_t i = size_; i > pos; i--) {
      keys_[i] = keys_[i-1];
      values_[i] = values_[i-1];
    }
    keys_[pos] = key_position;
    values_[pos] = value_position;
    // We return a reference for the value, but not the keys. The reason
    // to not add the reference for the keys is because it was already
    // created when we did the key_allocator_->Add(key) operation.
    object_allocator_->AddReference(value_position);
    size_++;
    return pos;
  }

  // If successful, this will add the key to the key_allocator,
  // the value to the object_allocator, and the key-value pair here.
  // We return the position where it was added, or max_size() otherwise.
  uint8_t Add(const char* key, const EltType& value) {
    uint32_t value_position = object_allocator_->Add(value);
    uint8_t ret_val  = Add(key, value_position);
    // There are two cases: the add was successful or the add failed.
    // If the add failed, we want to just remove the value we just added,
    // and doing DropReference will do that. If the add succeeded, then we
    // have two references to the object instead of one: The add to the
    // allocator adds the first reference, and invoking
    // Add(key, value_position) also adds one. In either case, we want to
    // do the same thing: Drop a reference.
    object_allocator_->DropReference(value_position);
    return ret_val;
  }

  inline uint8_t Add(uint16_t key_position, uint32_t value_position) {
    if (size_ == max_size_) {
      return max_size_;
    }

    bool exists = false;
    const char* key = key_allocator_->StringAt(key_position);
    uint8_t pos = (allow_dups_ ? GetPosition(key, value_position, &exists)
                               : GetPosition(key, &exists));
    if (!exists) {
      return max_size_;
    }
    for (uint8_t i = size_; i > pos; i--) {
      keys_[i] = keys_[i-1];
      values_[i] = values_[i-1];
    }
    keys_[pos] = key_position;
    values_[pos] = value_position;
    key_allocator_->AddReference(key_position);
    object_allocator_->AddReference(value_position);
    size_++;
    return pos;
  }

  void PutUsingArrayPosition(uint8_t pos, uint32_t update_value_position) {
    // pos must refer to a position actually in use.
    if (pos >=size_) {
      return;
    }

    uint32_t old_val = values_[pos];
    if (old_val != update_value_position) {
      object_allocator_->DropReference(old_val);
      object_allocator_->AddReference(update_value_position);
      values_[pos] = update_value_position;
    }
  }

  uint8_t Put(uint16_t key_position, uint32_t value_position) {
    bool exists = false;
    uint8_t pos = (allow_dups_ ?
                   GetPosition(key_position, value_position, &exists) :
                   GetPosition(key_position, &exists));
    if (exists) {
      uint32_t old_val = values_[pos];
      if (old_val != value_position) {
        object_allocator_->DropReference(old_val);
        object_allocator_->AddReference(value_position);
        values_[pos] = value_position;
      }
      return pos;
    }

    if (size_ == max_size_) {
      return max_size_;
    }

    for (uint8_t i = size_; i > pos; i--) {
      keys_[i] = keys_[i-1];
      values_[i] = values_[i-1];
    }
    keys_[pos] = key_position;
    values_[pos] = value_position;
    // We return a reference for the value, but not the keys. The reason
    // to not add the reference for the keys is because it was already
    // created when we did the key_allocator_->Add(key) operation.
    object_allocator_->AddReference(value_position);
    size_++;
    return pos;
  }

  bool Remove(const char* key) {
    bool exists = false;
    uint8_t pos = GetPosition(key, &exists);
    if (!exists) {
      return false;
    }

    uint16_t key_pos = keys_[pos];
    key_allocator_->DropReference(key_pos);
    uint32_t value_pos = values_[pos];
    object_allocator_->DropReference(value_pos);
    for (uint8_t i = pos + 1; i < size_; i++) {
      keys_[i-1] = keys_[i];
      values_[i-1] = values_[i];
    }
    return true;
  }

  inline bool Remove(uint16_t key_position) {
    return Remove(key_allocator_->StringAt(key_position));
  }

  inline uint8_t Size() const { return size_; }
  inline uint8_t MaxSize() const { return max_size_; }
  inline bool AllowsDuplicates() const { return allow_dups_; }

 private:
  StringAllocator* key_allocator_;
  ObjectAllocator<EltType>* object_allocator_;
  uint8_t max_size_;
  uint8_t size_;
  bool allow_dups_;
  uint16_t* keys_;
  uint32_t* values_;
};

} // namespace collection
} // namespace acumio

#endif // AcumioServer_flat_map_h
