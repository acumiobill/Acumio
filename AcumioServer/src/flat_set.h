#ifndef AcumioServer_flat_set_h
#define AcumioServer_flat_set_h
//============================================================================
// Name        : flat_set.h
// Author      : Bill Province (bill@acumio.com)
// Version     :
// Copyright   : Copyright (C) 2016 Acumio
// Description : This file contains the definition for FlatSet, which
//               is similar to std::set, but is implemented as a simple
//               vector instead of using a red-black tree. For
//               most operations - where the expected set-size should
//               be small - this will be preferred over std::set.
//               The array-list will be maintained in sorted order for
//               fast lookup times, but linear insertion times.
//
// TODO: Define test class for this.
//============================================================================

#include <algorithm> // std::lower_bound<IterType,CompareType> algorithm.
#include <functional> // std::less<T>
#include <memory> // std::unique_ptr<T>
#include <utility> // std::pair<T1,T2>
#include <vector> // std::vector<T>

namespace acumio {
namespace collection {
template <class T, class Compare = std::less<T> >
class FlatSet {
 public:
  class iterator {
   public:
    iterator(std::vector<T>& container, size_t location) :
        container_(container), location_(location), saved_tmp_(nullptr) {}
    iterator(std::vector<T>& container) : iterator(container, 0) {}
    iterator(const iterator& other) :
        iterator(other.container_, other.location_) {}

    inline iterator operator++() {
      location_++;
      return *this;
    }

    inline iterator operator--() {
      if (location_ == 0) {
        location_ = container_.size();
      } else {
        location_--;
      }
      return *this;
    }

    inline iterator operator++(int) {
      saved_tmp_.reset(new iterator(*this));
      location_++;
      return *saved_tmp_;
    }

    inline iterator operator--(int) {
      saved_tmp_.reset(new iterator(*this));
      if (location_ == 0) {
        location_ = container_.size();
      } else {
        location_--;
      }
      return *saved_tmp_;
    }

    inline T& operator*() { return container_[location_]; }
    inline T* operator->() { return &(container_[location]); }

    inline bool operator==(const iterator& other) const {
      return container_ == other.container_ && location_ == other.location_;
    }

    inline bool operator!=(const iterator& other) const {
      return container_ != other.container_ || location_ != other.location_;
    }

    inline size_t location() const { return location_; }
    inline const std::vector<T>& container() const { return container_;}

   private:
    std::vector<T>& container_;
    size_t location_;
    std::unique_ptr<iterator> saved_tmp_;
  };

  class const_iterator {
   public:
    const_iterator(const std::vector<T>& container, size_t location) :
        container_(container), location_(location), saved_tmp_(nullptr) {}
    const_iterator(const std::vector<T>& container) :
        const_iterator(container, 0) {}
    const_iterator(const const_iterator& other) :
        const_iterator(other.container_, other.location_) {}

    inline const_iterator operator++() {
      location_++;
      return *this;
    }

    inline const_iterator operator--() {
      if (location_ == 0) {
        location_ = container_.size();
      } else {
        location_--;
      }
      return *this;
    }

    inline const_iterator operator++(int) {
      saved_tmp_.reset(new const_iterator(*this));
      location_++;
      return *saved_tmp_;
    }

    inline const_iterator operator--(int) {
      saved_tmp_.reset(new const_iterator(*this));
      if (location_ == 0) {
        location_ = container_.size();
      } else {
        location_--;
      }
      return *saved_tmp_;
    }

    inline const T& operator*() { return container_[location_]; }
    inline const T* operator->() { return &(container_[location]); }

    inline bool operator==(const const_iterator& other) const {
      return container_ == other.container_ && location_ == other.location_;
    }

    inline bool operator!=(const const_iterator& other) const {
      return container_ != other.container_ || location_ != other.location_;
    }

    inline size_t location() const { return location_; }
    inline const std::vector<T>& container() const { return container_;}

   private:
    const std::vector<T>& container_;
    size_t location_;
    std::unique_ptr<const_iterator> saved_tmp_;
  };

  FlatSet(size_t capacity, const Compare& comp) : elements_(), cmp_(comp) {
    elements_.reserve(capacity);
  }

  FlatSet(size_t capacity) : elements_(), cmp_(Compare()) {
    elements_.reserve(capacity);
  }
  FlatSet(const Compare& comp) : FlatSet(256, comp) {}
  FlatSet() : FlatSet(256, Compare()) {}

  inline iterator begin() { return iterator(elements_, 0); }
  inline iterator rbegin() {
    return iterator(elements_,
                    (elements_.size() > 0) == 0 ? 0 : elements_.size() - 1);
  }

  inline iterator end() { return iterator(elements_, elements_.size()); }
  inline const_iterator begin() const { return const_iterator(elements_, 0); }
  inline const_iterator rbegin() const {
    return const_iterator(elements_,
        (elements_.size() > 0) == 0 ? 0 : elements_.size() - 1);
  }

  inline const_iterator end() const {
    return const_iterator(elements_, elements_.size());
  }

  // Returns reference to smallest value v such that v >= val. Alternatively,
  // if all values v < val then returns end().
  const_iterator lower_bound(const T& val) const {
    // checks that either set is empty or that smallest elt >= val.
    if (elements_.size() == 0 || !cmp_(elements_[0], val)) {
      return begin();
    }

    // assert: elements_[0] < val
    size_t lowest = 0;
    size_t highest = elements_.size() - 1;

    if (cmp_(elements_[highest], val)) {
      return end();
    }

    // assert elements_[lowest] < val <= elements_[highest]
    size_t mid = ((lowest + highest) >> 1);
    while (mid != lowest) {
      // loop invariant: elements_[lowest] < val <= elements_[highest]
      // Guaranteed at this point in the loop before the if-statement
      // below.
      if (cmp_(elements_[mid], val)) {
        lowest = mid;
      } else {
        highest = mid;
      }
      mid = ((lowest + highest) >> 1);
    }
    return const_iterator(elements_, highest);
  }

  // Returns reference to smallest value v such that v >= val. Alternatively,
  // if all values v < val then returns end().
  iterator lower_bound(const T& val) {
    // checks that either set is empty or that smallest elt >= val.
    if (elements_.size() == 0 || !cmp_(elements_[0], val)) {
      return begin();
    }

    // assert: elements_[0] < val
    size_t lowest = 0;
    size_t highest = elements_.size() - 1;

    if (cmp_(elements_[highest], val)) {
      return end();
    }

    // assert elements_[lowest] < val <= elements_[highest]
    size_t mid = ((lowest + highest) >> 1);
    while (mid != lowest) {
      // loop invariant: elements_[lowest] < val <= elements_[highest]
      // Guaranteed at this point in the loop before the if-statement
      // below.
      if (cmp_(elements_[mid], val)) {
        lowest = mid;
      } else {
        highest = mid;
      }
      mid = ((lowest + highest) >> 1);
    }
    return iterator(elements_, highest);
  }

  // Returns reference to smallest value v such that v > val. Alternatively,
  // if all values v <= val then returns end().
  const_iterator upper_bound(const T& val) const {
    // checks that either set is empty or that smallest elt < val.
    if (elements_.size() == 0 || cmp_(val, elements_[0])) {
      return begin();
    }

    // assert: ! (val < elements_[0])
    //      ==> elements_[0] <= val
    size_t lowest = 0;
    size_t highest = elements_.size() - 1;

    if (!cmp_(val, elements_[highest])) {
      // assert: !(val < highest elt)
      //  ==>     val >= highest elt
      return end();
    }

    // assert elements_[lowest] <= val < elements_[highest]
    size_t mid = ((lowest + highest) >> 1);
    while (mid != lowest) {
      // loop invariant: elements_[lowest] <= val < elements_[highest]
      // Guaranteed at this point in the loop before the if-statement
      // below.
      if (cmp_(val, elements_[mid])) {
        highest = mid;
      } else {
        lowest = mid;
      }
      mid = ((lowest + highest) >> 1);
    }
    return const_iterator(elements_, highest);
  }

  std::pair<iterator,bool> insert(const T& val) {
    iterator iter = lower_bound(val);
    if (iter == end()) {
      elements_.push_back(val);
      return std::pair<iterator,bool>(iter, true);
    }
    if (*iter == val) {
      return std::pair<iterator,bool>(iter, false);
    }
    size_t location = iter.location();
    for (size_t index = elements_.size(); index > location; index--) {
      elements_[index] = elements_[index - 1]; 
    }
    elements_[location] = val;
    return std::pair<iterator,bool>(iter, true);
  }

  size_t size() const {
    return elements_.size();
  }

  size_t count(const T& val) const {
    iterator location = lower_bound(val);
    return (location == end() || (*location != val)) ? 0 : 1;
  }

  size_t erase(const T& val) {
    iterator location = lower_bound(val);
    if (location == end() || *location != val) {
      return 0;
    }
    // assert: *location == val
    for (size_t i = location.location(); i < elements_.size() - 1; i++) {
      elements_[i] = elements_[i+1];
    }
    elements_.pop_back();
    return 1;
  }
  
 private:
  std::vector<T> elements_;
  Compare cmp_;
};

} // namespace collection
} // namespace acumio

#endif // AcumioServer_flat_set_h
