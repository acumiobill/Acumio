#ifndef AcumioServer_protobuf_iterator_h
#define AcumioServer_protobuf_iterator_h
//============================================================================
// Name        : protobuf_iterator.h
// Author      : Bill Province (bill@acumio.com)
// Version     :
// Copyright   : Copyright 2016
// Description : An implementation of an stl-iterator for protos that
//               hides the awkward
//               google::protobuf::iternal::RepeatedPtrIterator
//               class in favor of something a bit easier to digest.
//               
// There is both a "const" and non-"const" version here.
//
// Typical usage:
//  Given: a variable "my_proto" of type Proto, with a repeated attribute
//         called "repeated_attribute", here is how to iterate over all
//         the elementsL
//
//         ProtoIterator<std::string> end_iter(
//             my_proto.repeated_attribute().end());
//         ProtoIterator<std::string> begin_iter(
//             my_proto.repeated_attribute().begin());
//         for (ProtoIterator<std::string> it = begin_iter;
//              it != end_iter; it++) {
//           std::string& current_val = *it;
//           <do something here>
//         }
//============================================================================

#include <google/protobuf/repeated_field.h>
#include <iterator>

namespace acumio {
namespace proto {
template <class E>
class ProtoIterator : public std::iterator<std::forward_iterator_tag, E> {
 typedef google::protobuf::internal::RepeatedPtrIterator<E> GoogleIterator;
 public:
  ProtoIterator(GoogleIterator delegate) : delegate_(delegate){}
  ProtoIterator(const ProtoIterator& other) : delegate_(other.delegate_) {}
  ~ProtoIterator() {}

  inline E& operator*() { return *delegate_; }

  // pre-increment
  inline ProtoIterator<E>& operator++() {
    ++delegate_;
    return *this;
  }

  // post-increment/decrement
  inline ProtoIterator<E> operator++(int) {
    ProtoIterator<E> tmp(*this);
    delegate_++;
    return tmp;
  }

  inline bool operator==(const ProtoIterator<E>& other) {
    return delegate_ == other.delegate_;
  }

  inline bool operator!=(const ProtoIterator<E>& other) {
    return delegate_ != other.delegate_;
  }

 private:
  GoogleIterator delegate_;
};

template <class E>
class ConstProtoIterator :
    public std::iterator<std::forward_iterator_tag, const E> {
 typedef google::protobuf::internal::RepeatedPtrIterator<const E>
     GoogleIterator;
 public:
  ConstProtoIterator(GoogleIterator delegate) : delegate_(delegate) {}
  ConstProtoIterator(const ConstProtoIterator& other) :
      delegate_(other.delegate_) {}
  ~ConstProtoIterator() {}
  inline const E& operator*() { return *delegate_; }

  // pre-increment/decrement
  inline ConstProtoIterator<E>& operator++() {
    ++delegate_;
    return *this;
  }

  // post-increment/decrement
  inline ConstProtoIterator<E> operator++(int) {
    ConstProtoIterator<E> tmp(*this);
    delegate_++;
    return tmp;
  }

  inline bool operator==(const ConstProtoIterator<E>& other) {
    return delegate_ == other.delegate_;
  }

  inline bool operator!=(const ConstProtoIterator<E>& other) {
    return delegate_ != other.delegate_;
  }

 private:
  GoogleIterator delegate_;
};

} // namespace proto
} // namespace acumio
#endif // AcumioServer_protobuf_iterator_h
