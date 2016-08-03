#ifndef AcumioServer_rope_piece_h
#define AcumioServer_rope_piece_h
//============================================================================
// Name        : rope_piece.h
// Author      : Bill Province (bill@acumio.com)
// Version     :
// Copyright   : Copyright (C) 2016 Acumio
// Description : the RopePiece class acts in a fashion similar to a Rope and a
//               StringPiece.
//
//               Like the StringPiece class, it references existing character
//               memory space rather than managing the space as its own.
//
//               Like the Rope class, it handles the string in segments rather
//               than having a single long strand of memory where the contents
//               are stored. The SGI reference implementation of a Rope uses
//               an internal rope_node class. That is intentionally discarded
//               here, as the Rope class itself effectively acts as a
//               RopeNode. In addition, rather than being a binary tree,
//               the RopePiece has arbitrary fan-out at any one node.
//
//               The RopePiece also acts as an immutable class, unlike existing
//               rope class implementations.
//
//               The implementation here is only partial, serving the need of
//               forming efficient key-iteration over a (Patricia-) Trie
//               data structure. This may change in the future as the role
//               expands.
//============================================================================

#include "util_constants.h" // needed for LETTER_STRINGS[] array.

#include <memory>
#include <stack>
#include <string>

#include "iterators.h"

namespace acumio {
namespace string {

using acumio::collection::BasicIterator;

class RopePiece {
 public:

  class Iterator : public BasicIterator<char> {
   public:
    Iterator();
    Iterator(const RopePiece* container, uint32_t index);
    Iterator(const Iterator& other);
    ~Iterator();

    BasicIterator<char>* Clone() const;

    // pre-increment/decrement
    BasicIterator<char>& operator++();
    BasicIterator<char>& operator--();
    // post-increment/decrement
    BasicIterator<char>& operator++(int);
    BasicIterator<char>& operator--(int);
    // Dereference
    const char& operator*() const;
    const char* operator->() const;

    // Equality comparison
    bool operator==(const BasicIterator<char>& other) const;

   private:
    struct StackElement {
      StackElement(const RopePiece* rope, uint32_t index) :
          rope_(rope), index_(index) {}
      StackElement() : rope_(nullptr), index_(0) {}
      StackElement(const StackElement& other) :
          rope_(other.rope_), index_(other.index_) {}
      ~StackElement() {}
      inline bool operator==(const StackElement& other) const {
        return rope_ == other.rope_ && index_ == other.index_;
      }

      const RopePiece* rope_;
      uint32_t index_;
    };

    std::stack<StackElement> stack_;
    std::unique_ptr<Iterator> saved_tmp_;
  };

  friend class Iterator;

  // Generated RopePiece is an empty string.
  RopePiece();

  // s is assumed to reference content that has a lifetime longer
  // then the constructed RopePiece.
  RopePiece(const char* s);

  // s is assumed to reference content that has a lifetime longer
  // then the constructed RopePiece.
  RopePiece(const std::string& s);

  // Same as RopePiece(string_(LETTER_STRINGS[(uint8_t) c]));
  // where LETTER_STRINGS is defined in util_constants.h.
  RopePiece(char c);

  // The parameter r is expected to have a lifetime exceeding the lifetime of
  // the constructed RopePiece.
  RopePiece(const RopePiece& r);

  // The ownership for prefix and suffix will be put into a std::shared_ptr in
  // the RopePiece class, and as such, the onwership of the prefix and suffix
  // will be shared by the caller.
  RopePiece(const RopePiece* prefix, const RopePiece* suffix);

  RopePiece(std::shared_ptr<const RopePiece> prefix,
            std::shared_ptr<const RopePiece> suffix);

  // The prefix and suffic parameters are expected to have a lifetime
  // exceeding the lifetime of the constructed RopePiece.
  // Same as: RopePiece(&prefix, &suffix, false, false);
  RopePiece(const RopePiece& prefix, const RopePiece& suffix);

 ~RopePiece();

  // The parameter r is expected to have a lifetime exceeding the lifetime of
  // the copied object. We want this in addition to the copy constructor to
  // handle the case where we are overriding an existing RopePiece and need
  // to delete the elements held.
  const RopePiece& operator=(const RopePiece& r);

  // Returns character that would be at given position. If position >= size(),
  // returns '\0'.
  char CharAt(uint32_t position) const;

  std::string ToString() const;

  // Standard compare ability: return less than 0 if this RopePiece is
  // lexically less than other, 0 if equal and greater than 1 if lexically
  // greater.
  int compare(const RopePiece& other) const;

  // Standard compare ability: return less than 0 if this RopePiece is
  // lexically less than const char* other, 0 if equal and greater than 1 if
  // lexically greater.
  int compare(const char* other) const;
  // Standard compare ability: return less than 0 if this RopePiece is
  // lexically less than std::string other, 0 if equal and greater than 1 if
  // lexically greater.
  inline int compare(const std::string&  other) const {
    return compare(other.c_str());
  }
  
  inline uint32_t size() const { return length_; }
  inline uint32_t length() const { return length_; }

  inline char operator[](uint32_t position) const { return CharAt(position); }

  const Iterator begin() const;
  const Iterator end() const;
  Iterator IteratorFromIndex(uint32_t index) const;

 private:
  /********************Private Member Functions *****************************/
  // Copy contents of RopePiece into the provided buffer.
  // Buffer is assumed to be pre-allocated to at least length_ + 1.
  // If allocation length is greater, this will copy all except the
  // null-termination.
  void CopyToBuffer(char* buffer) const;

  /********************Private Member Variables *****************************/
  // The structure will either be string_ not null with prefix_ and suffix_
  // being null, or with prefix_ and suffix_ being not null and string_ being
  // null.
  // We pre-compute the length parameter.
  // The RopePiece will sometimes "own" the left or right Ropes, as indicated
  // by the left/right ownership flags.
  const char* string_;
  std::shared_ptr<const RopePiece> prefix_;
  std::shared_ptr<const RopePiece> suffix_;
  uint32_t length_;
};

} // namespace string
} // namespace acumio

#endif // AcumioServer_rope_piece_h
