#ifndef AcumioServer_object_allocator_h
#define AcumioServer_object_allocator_h
//============================================================================
// Name        : object_allocator.h
// Author      : Bill Province (bill@acumio.com)
// Version     :
// Copyright   : Copyright (C) 2016 Acumio
// Description : A mechanism for allocating objects of a uniform type.
//               The intent is to have access to the objects themselves
//               referenceable by a uint32_t - which will be small enough
//               to make their use in an index cheap. In this way, an index
//               can refer to the "values" portion as a uint32_t while
//               we store the objects themselves as a massive pre-allocated
//               array.
//
//               Use care when selecting an initial capacity. the intent here
//               is for a somewhat large in-memory store. The initial capacity
//               should be set to accomodate the expected size of the store.
//               If the arrays have to grow, the re-allocation time might
//               make the value of using this class questionable.
//
//               TODO: Allocate chunks of capacity instead of having a single
//               large capacity block. See the TODO comment in the private
//               member variables section.
//               possible allocation would then be 2048*2048 =~ 4 million.
//============================================================================

#include <stack>
#include <stdint.h>
#include <string>
#include <vector>

namespace acumio {
namespace collection {

// EltType must have a no-arg constructor for this to work.
template <typename EltType>
class ObjectAllocator {
 public:
  const uint16_t MAX_REFERENCE_COUNT = UINT16_C(65534);

  ObjectAllocator(uint32_t initial_capacity = UINT32_C(16384)) :
      elements_(), reference_counts_(), free_list_() {
    elements_.reserve(initial_capacity);
    reference_counts_.reserve(initial_capacity);
    free_list_.push(UINT32_C(0));
  }

  ~ObjectAllocator() {}

  inline const EltType& ObjectAt(uint32_t position) const {
    return elements_[position];
  }

  inline EltType& ModifiableObjectAt(uint32_t position) {
    return elements_[position];
  }

  uint32_t Add(const EltType& object) {
    uint32_t ret_val = free_list_.top();
    free_list_.pop();
    if (free_list_.empty()) {
      elements_.push_back(object);
      reference_counts_.push_back(UINT16_C(1));
      free_list_.push(elements_.size());
    } else {
      elements_[ret_val] = object;
      reference_counts_[ret_val] = UINT16_C(1);
    }

    return ret_val;
  }

  // Returns the total number of references for the object at the given
  // position. If there currently are no references at that position,
  // this operation is ignored, and we return 0. If there are currently
  // MAX_REFERENCE_COUNT references, this again is ignored, and 0 is
  // returned. Otherwise, the return is the total reference count after
  // we add one more for the current request.
  //
  // The operation is undefined if position does not refer to an actual
  // object.
  uint16_t AddReference(uint32_t position) {
    if (position >= reference_counts_.size()) {
      return 0;
    }
    int ret_val = ++(reference_counts_[position]);
    if (ret_val == 1) {
      reference_counts_[position] = 0;
      return 0;
    } else if (ret_val > MAX_REFERENCE_COUNT) {
      reference_counts_[position] = MAX_REFERENCE_COUNT;
      return 0;
    }

    return ret_val;
  }

  // Returns the total number of references to the given position after
  // dropping one. If the total number of references at the position was
  // previously 0, this is a no-op, with 0 returned. If the total
  // reference count goes to 0, this is effectively removed.
  uint16_t DropReference(uint16_t position) {
    if (position >= reference_counts_.size()) {
      return 0;
    }
    uint16_t* value = &(reference_counts_[position]);
    switch (*value) {
      case 0:
        return 0;
      case 1: 
        *value = 0;
        if (position == elements_.size() - 1) {
          if (free_list_.top() == elements_.size()) {
            free_list_.pop();
          }
          elements_.pop_back();
          reference_counts_.pop_back();
          free_list_.push(elements_.size());
          return 0;
        }
      default:
        *value = *value - 1;
    }
    return *value;
  }

  inline uint16_t ReferenceCount(uint32_t position) const {
    return reference_counts_[position];
  }

  inline uint32_t size() {
    return elements_.size() + 1 - free_list_.size();
  }

  inline uint32_t ImpossiblePosition() const {
    return UINT32_MAX;
  }

 private:
  // TODO: Modify elements_ and reference_counts_ to:
  //  std::vector<std::vector<EltType>> elements_ and
  //  std::vector<std::vector<uint16_t>> reference_counts_ respectively.
  // Currently, we reserve an initial vector size, and rely on standard
  // vector growth to handle capacity. The idea with the double vectoring
  // is to have an initial capacity of V vectors, and each of the vectors
  // could be allowed to reserve initially 1 element, but when growth is
  // needed, it could reserve M elements. This will allow for V*M elements
  // before needing to worry about re-allocation.
  // M should be large enough to give efficiency but small enough that we
  // won't get into trouble with memory fragmentation. Setting V and M
  // to 2048 each allows for approx. 4 million elements total. The downside
  // of this approach is that references to the elements will require
  // an extra pointer hop.
  std::vector<EltType> elements_;
  std::vector<uint16_t> reference_counts_;
  std::stack<uint32_t> free_list_;
};

} // namespace collection
} // namespace acumio

#endif // AcumioServer_object_allocator_h

