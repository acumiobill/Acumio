#ifndef AcumioServer_string_allocator_h
#define AcumioServer_string_allocator_h
//============================================================================
// Name        : string_allocator.h
// Author      : Bill Province (bill@acumio.com)
// Version     :
// Copyright   : Copyright (C) 2016 Acumio
// Description : A mechanism for allocating strings in a managed block of
//               space. The purpose of doing this - as opposed to just
//               using the heap to manage strings - is to keep the string
//               content close together - hopefully, using L2 cache.
//
//               This should only be considered in cases where you are willing
//               to pay a complexity cost in order to get a performance
//               advantage.
//
//               Note that this is not designed to work with the std::allocator
//               template, despite the similarity in name. This is because
//               this is intended for use with highly specialized containers.
//
//               In addition, though this is called a "StringAllocator", it
//               uses the C notion of a string as a null-terminated array
//               of characters rather than using the C++ std::string library.
//               Adaptor methods working with std::string are provided, but
//               the mechanism works with low-level character arrays.
//
//               The mechanism uses a fixed-size array created at construction
//               time. Typically, when adding a string, the process returns a
//               position in the array.  However, if the fixed-size array would
//               be exceeded by the attempted string-add, the process instead
//               returns the max_size value (Clearly, the string cannot start
//               at max_size, so this should be seen as an error codition).
//
//               There is no transaction or thread awareness in this class.
//               As such, making modifications to it should only be done in
//               a context where there is no possibility of multi-threaded
//               modifications (guarding modification or read operations with
//               a SharedMutex should be sufficent).
//============================================================================

#include <stdint.h>
#include <string>
#include <vector>

namespace acumio {
namespace collection {

class StringAllocator {
 public:
  const uint8_t MAX_REFERENCE_COUNT = UINT8_C(255);

  StringAllocator(uint16_t max_size);
  ~StringAllocator();

  inline uint16_t max_size() const { return max_size_; }

  inline const char* StringAt(uint16_t position) const {
    return key_list_ + position;
  }

  // If unable to fulfill request to add string, returns max_size_.
  // Otherwise, returns offset position where s is stored.
  uint16_t Add(const char* s);

  inline uint16_t Add(const std::string& s) { return Add(s.c_str()); }

  // Returns the total number of references for the string at the given
  // position. If there currently are no references at that position,
  // this operation is ignored, and we return 0. If there are currently
  // MAX_REFERENCE_COUNT references, this again is ignored, and 0 si
  // returned. Otherwise, the return is the total reference count after
  // we add one more for the current request.
  uint8_t AddReference(uint16_t position);

  // Returns the total number of references to the given position after
  // dropping one. If the total number of references at the position was
  // previously 0, this is a no-op, with 0 returned.
  uint8_t DropReference(uint16_t position);

  inline uint8_t ReferenceCount(uint16_t position) const {
    return reference_counts_[position];
  }

 private:
  struct FreeKey {
    FreeKey() : FreeKey(UINT16_C(0), UINT16_C(0)) {}
    FreeKey(const FreeKey& o)  :
        FreeKey(o.location, o.hole_length) {}
    FreeKey(uint16_t l, uint16_t hl) : location(l), hole_length(hl) {}
    uint16_t location;
    uint16_t hole_length;
  };

  uint16_t max_size_;
  uint16_t space_used_;
  char* key_list_;
  uint8_t* reference_counts_;
  std::vector<FreeKey> free_key_list_;
};

} // namespace collection
} // namespace acumio

#endif // AcumioServer_string_allocator_h

