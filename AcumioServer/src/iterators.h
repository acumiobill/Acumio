#ifndef AcumioServer_iterators_h
#define AcumioServer_iterators_h
//============================================================================
// Name        : iterators.h
// Author      : Bill Province (bill@acumio.com)
// Version     :
// Copyright   : Copyright (C) 2016 Acumio
// Description : Provides some basic templated base-class iterators of the
//               type we expect to use internally. The std template library
//               does not provide base classes for their iterators.
//               The reason is possibly because they expect the return-types
//               for the iterators to match the iterator-class for the
//               increment/decrement and navigation processes. However,
//               for most needs, this is actually not helpful.
//               We provide here the following:
//
//               BasicIterator :
//                   A base class for our common Iterator needs. This is
//                   templated based on the desired return type when doing
//                   dereference operations. In addition, our BasicIterator
//                   will support the idea of holding a read-only lock on a
//                   mutex for its life-time. (This is optional, and decided
//                   in the constructor).
//               DelegatingIterator:
//                   An implementing template class based on the idea of
//                   delegating the navigation to another iterator.
//                              
//============================================================================

#include <iterator>
#include "shared_mutex.h"

namespace acumio {
namespace collection {

using acumio::transaction::SharedLock;
using acumio::transaction::SharedMutex;

template <typename EltType> class BasicIterator :
    public std::iterator<std::bidirectional_iterator_tag, EltType> {
 public:
  BasicIterator() : lock_(nullptr) {}
  BasicIterator(SharedMutex* guard) : lock_(nullptr) {
    lock_ = new SharedLock(*guard);
  }
  BasicIterator(const BasicIterator& other) : lock_(nullptr) {
    if (other.lock_ != nullptr) {
      lock_ = new SharedLock(other.lock_->GetMutex());
    }
  }

  virtual ~BasicIterator() {
    delete lock_;
  }

  // We need to have a type of "virtual copy constructor." You cannot do
  // that directly in the language, but we can build in the idea here.
  // The caller is expected to take ownership of the created value.
  virtual BasicIterator* Clone() const = 0;

  // pre-increment/decrement
  virtual BasicIterator& operator++() = 0;
  virtual BasicIterator& operator--() = 0;
  // post-increment/decrement
  virtual BasicIterator& operator++(int) = 0;
  virtual BasicIterator& operator--(int) = 0;
  // Dereference
  virtual const EltType& operator*() const = 0;
  virtual const EltType* operator->() const = 0;

  // Equality comparison
  virtual bool operator==(const BasicIterator<EltType>& other) const = 0;
  inline bool operator!=(const BasicIterator<EltType>& other) const {
    return !(this->operator==(other));
  }

  // Guard access
  inline SharedMutex* guard() const {
    return lock_ == nullptr ? nullptr : &(lock_->GetMutex());
  }

 private:
  mutable SharedLock* lock_;
};

template <typename EltType>
class DelegatingIterator : public BasicIterator<EltType> {
 public:
  DelegatingIterator(std::shared_ptr<BasicIterator<EltType>> delegate) :
      BasicIterator<EltType>(), delegate_(delegate) {}

  DelegatingIterator(std::shared_ptr<BasicIterator<EltType>> delegate,
                     SharedMutex* guard) :
      BasicIterator<EltType>(guard), delegate_(delegate) {}

  DelegatingIterator(const DelegatingIterator& other) :
      BasicIterator<EltType>(static_cast<const BasicIterator<EltType>&>(other)),
      delegate_(other.delegate_) {}

  ~DelegatingIterator() {}

  BasicIterator<EltType>* Clone() const {
    return new DelegatingIterator(*this);
  }

  // Here, we diverge a bit from the standard templates, because our return
  // type is a BasicIterator& instead of a DelegatingIterator&.
  inline BasicIterator<EltType>& operator++() {
    (*delegate_)++;
    return *this;
  }

  inline BasicIterator<EltType>& operator--() {
    (*delegate_)--;
    return *this;
  }

  // post-increment/decrement
  BasicIterator<EltType>& operator++(int) {
    saved_tmp_.reset(new DelegatingIterator(*this));
    (*delegate_)++;
    return *saved_tmp_;
  }

  BasicIterator<EltType>& operator--(int) {
    saved_tmp_.reset(new DelegatingIterator(*this));
    (*delegate_)--;
    return *saved_tmp_;
  }

  inline const EltType& operator*() const {
    return **delegate_;
  }

  inline const EltType* operator->() const {
    return delegate_->operator->();
  }

  inline bool operator==(const BasicIterator<EltType>& other) const {
    const DelegatingIterator* delegating_other =
      dynamic_cast<const DelegatingIterator*>(&other);
    return (delegating_other != nullptr &&
            *delegate_ == *(delegating_other->delegate_));
  }

 private:
  std::shared_ptr<BasicIterator<EltType>> delegate_;
  std::unique_ptr<DelegatingIterator> saved_tmp_;
};

} // namespace collection
} // namespace acumio

#endif // AcumioServer_iterators_h
