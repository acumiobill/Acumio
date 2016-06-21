#ifndef AcumioServer_tx_aware_flat_map_h
#define AcumioServer_tx_aware_flat_map_h
//============================================================================
// Name        : tx_aware_flat_map.h
// Author      : Bill Province (bill@acumio.com)
// Version     :
// Copyright   : Copyright (C) 2016 Acumio
// Description : A versioned and transaction-aware variant of a FlatMap.
//               A FlatMap is defined here as just a flat array of string-based
//               keys with associated uint32_t values. Ordering of the keys is
//               maintained via insertion sort.
//
//               While insertion sort has limited scalability, for small
//               values, it will be effective due to the locality of the
//               data.
//
//               This structure comes with some built-in boundaries that
//               prevent growth beyond a fixed size. If adding a particular
//               data element would cause the map to exceed the size
//               limit, we reject the operation. The advantage of
//               having a maximal size is that we can use the encounter with
//               a maximal size as a trigger to 'burst' in a Burst-Trie map.
//
//               To handle transaction-awareness, we version each node of
//               our array. In addition, we supply transaction-aware APIs
//               so that no modification or read can occur without knowing
//               the modifying transaction or the point-in-time in which
//               to conduct a read operation.
//
//               Whether or not duplicate keys are allowed is defined by
//               the constructor parameter "allow_dups". However, this only
//               defined duplication with respect to keys. So, we might allow
//               for the same key to reference multiple different values,
//               but we reject having the same key-value pair represented
//               multiple times.
//
//               In the course of performing a write-transaction that depends
//               on reading data, having a write-intent lock should be
//               maintained externally to this structure. Note that
//               effectively, this implies that modifications are
//               single-threaded.
//============================================================================

//#include <atomic>
//#include <functional>
#include <grpc++/support/status.h>
#include <iterator>
#include <mutex>
#include <string>
#include <vector>
#include "flat_map.h"
#include "shared_mutex.h"
#include "string_allocator.h"
#include "transaction.h"
#include "tx_aware.h"

namespace acumio {
namespace collection {
using acumio::transaction::ExclusiveLock;
using acumio::transaction::SharedLock;
using acumio::transaction::SharedMutex;
using acumio::transaction::Transaction;
using acumio::transaction::WriteTransaction;
using acumio::transaction::TxAware;

// A TrackingTransaction identifies modified elements associated with
// a transaction. So for example, if we modify nodes 18, 12, and 9 in
// a single transaction, the Tracking transaction will contain these
// listed nodes.

template <typename EltType>
class TxAwareFlatMap {
 public:
  typedef typename FlatMap<EltType>::Iterator _InnerIter;
  typedef typename FlatMap<EltType>::IteratorElement IteratorElement;
  class Iterator :
      public std::iterator<std::bidirectional_iterator_tag, IteratorElement> {
   public:
    Iterator(const _InnerIter& delegate, SharedMutex* guard) :
        delegate_(delegate), lock_(*guard) {}
    Iterator(const Iterator& copy) :
      delegate_(copy.delegate_), lock_(&(copy.lock_.GetMutex())) {}
    ~Iterator() {}

    // pre-increment/decrement
    Iterator& operator++() {
      delegate_++;
      return *this;
    }

    Iterator& operator--() {
      delegate_--;
      return *this;
    }

    // post-increment/decrement
    Iterator operator++(int) {
      saved_tmp_.reset(new Iterator(*this));
      delegate_++;
      return *saved_tmp_;
    }

    Iterator& operator--(int) {
      saved_tmp_.reset(new Iterator(*this));
      delegate_--;
      return *saved_tmp_;
    }

    bool operator==(const Iterator& other) const {
      return delegate_ == other.delegate_;
    }

    bool operator!=(const Iterator& other) const {
      return delegate_ != other.delegate_;
    }

    IteratorElement operator*() {
      return *delegate_;
    }

    const IteratorElement* operator->() {
      return delegate_.operator->();
    }

   private:
    _InnerIter delegate_;
    SharedLock lock_;
    std::unique_ptr<Iterator> saved_tmp_;
  };

  /************ Finally: the public methods. ************/ 
  // Max space indicates the max space available to write keys. It is *not*
  // the total memory consumed, though it comprises the largest fraction of
  // the total memory consumed.
  //
  // The max_size indicates the maximum number of elements to maintain
  // in the map.
  //
  // As a general rule, we will allow 16 elements as max_size and 2k bytes as
  // max_key_space. Note that key_space needs to be sufficient to allow both
  // current and historical versions.
  // TODO: Verify best numbers experimentally.
  //
  // The cleanup_nanos specifies when we can potentially discard older versions,
  // as any read-transactions still pending should presumably time out.
  //
  // The allow_dups parameter determines if duplicate keys are accepted or
  // rejected. Having said that, having a duplicate of both key and value is
  // *never* allowed.
  TxAwareFlatMap(ObjectAllocator<EltType>* object_allocator,
                 uint16_t max_key_space, uint8_t max_size,
                 uint64_t cleanup_nanos, uint64_t create_time,
                 bool allow_dups = false) : cleanup_nanos_(cleanup_nanos),
      max_space_(max_key_space), max_size_(max_size), allow_dups_(allow_dups),
      key_allocator_(new StringAllocator(max_key_space)),
      object_allocator_(object_allocator), elements_(nullptr) {
    FlatMap<EltType> not_present_value;
    elements_ = new VersionArray(not_present_value, create_time);
  }

  // A FlatMap in this form is not actually usable. However, we have this
  // so we could create arrays of TxAwareFlatMap objects.
  TxAwareFlatMap() : TxAwareFlatMap(nullptr, 0, 0, 0, 0, false) {}

  ~TxAwareFlatMap() {
    delete elements_;
  }

  grpc::Status Add(const char* key, uint32_t value_position,
                   const Transaction* tx, uint64_t edit_time) {
    acumio::transaction::ExclusiveLock guard(guard_);
    grpc::Status check = VerifyTxEditTime(tx, edit_time);
    if (!check.ok()) {
      return check;
    }
    bool exists_at_time = false;
    const FlatMap<EltType>& current_array =
        elements_->Get(edit_time, &exists_at_time);
    if (exists_at_time && current_array.Size() == max_size_) {
      return grpc::Status(grpc::StatusCode::OUT_OF_RANGE,
                          "FlatMap has reached max size.");
    }

    bool exists_key = false;
    uint8_t new_elt_pos = (allow_dups_ ?
                           current_array.GetPosition(key, value_position,
                                                     &exists_key) :
                           current_array.GetPosition(key, &exists_key));
    if (exists_key) {
      return grpc::Status(grpc::StatusCode::ALREADY_EXISTS,
                          "There is already an entry with the given key.");
    }

    uint16_t key_pos = key_allocator_->Add(key);
    if (key_pos == max_size_) {
      return grpc::Status(grpc::StatusCode::OUT_OF_RANGE,
                          "FlatMap has reached max key space.");
    }

    FlatMap<EltType> new_version(current_array, key_pos, value_position,
                                 new_elt_pos);
    // The key_allocator_->Add(key) and the use of the above constructor
    // would double-count the addition of the key. For this reason, we
    // drop one of the references.
    key_allocator_->DropReference(key_pos);
    grpc::Status result = elements_->Set(new_version, tx, edit_time);
    if (!result.ok()) {
      key_allocator_->DropReference(key_pos);
    }
    return result;
  }

  inline grpc::Status Add(const std::string& key, uint32_t value,
                          const Transaction* tx, uint64_t edit_time) {
    return Add(key.c_str(), value, tx, edit_time);
  }

  WriteTransaction::OpFunction AddCallback(const char* key, uint32_t value,
                                           uint64_t edit_time) {
    // This trick is to address the fact that the Add method is overloaded.
    // We disambiguate which add method we mean by first creating a fn-pointer
    // to reference exactly the add method we wish. The type parameters tell
    // the compiler which Add method to use.

    // fn is a pointer to a member function of TxAwareFlatMap<EltType> that
    // accepts params const char*, uint32_t, Transaction*, and uint64_t and
    // returns a status object. We initialize it to the Add method of our map.
    grpc::Status (TxAwareFlatMap::*fn)(const char*, uint32_t,
        const Transaction*, uint64_t) = &TxAwareFlatMap<EltType>::Add;

    // Note that if the Add method were not ambiguous, we could simply replace
    // fn with &TxAwareFlatMap<EltType>::Add.
    return std::bind(fn, this, key, value, std::placeholders::_1, edit_time);
  }
 
  grpc::Status Replace(const char* key, uint32_t value, const Transaction* tx,
                       uint64_t edit_time) {
    acumio::transaction::ExclusiveLock guard(guard_);
    grpc::Status check = VerifyTxEditTime(tx, edit_time);
    if (!check.ok()) {
      return check;
    }
    bool exists_at_time = false;
    const FlatMap<EltType>& current_array =
        elements_->Get(edit_time, &exists_at_time);
    if (!exists_at_time) {
      return grpc::Status(grpc::StatusCode::NOT_FOUND,
                          "Could not find key to replace.");
    }

    bool exists_key = false;
    uint8_t new_elt_pos = (allow_dups_ ?
                           current_array.GetPosition(key, value,
                                                     &exists_key) :
                           current_array.GetPosition(key, &exists_key));
    if (!exists_key) {
      return grpc::Status(grpc::StatusCode::NOT_FOUND,
                          "Could not find key to replace.");
    }

    FlatMap<EltType>& update_array(current_array);
    update_array.PutUsingArrayPosition(new_elt_pos, value);

    return elements_->Set(update_array, tx, edit_time);
  }

  inline grpc::Status Replace(const std::string& key, uint32_t value,
                              const Transaction* tx, uint64_t edit_time) {
    return Replace(key.c_str(), value, tx, edit_time);
  }

  WriteTransaction::OpFunction ReplaceCallback(
      const char* key, uint32_t value, uint64_t edit_time) {
    grpc::Status (TxAwareFlatMap::*fn)(const char*, uint32_t,
        const Transaction*, uint64_t) = &TxAwareFlatMap<EltType>::Replace;
    return std::bind(fn, this, key, value, std::placeholders::_1, edit_time);
  }

  grpc::Status Remove(const char* key, const Transaction* tx,
                      uint64_t edit_time) {
    if (allow_dups_) {
      return grpc::Status(grpc::StatusCode::FAILED_PRECONDITION,
                          "This method is not valid if allow_dups is allowed.");
    }
    acumio::transaction::ExclusiveLock guard(guard_);
    grpc::Status check = VerifyTxEditTime(tx, edit_time);
    if (!check.ok()) {
      return check;
    }
    bool exists_at_time = false;
    const FlatMap<EltType>& current_array =
        elements_->Get(edit_time, &exists_at_time);
    if (!exists_at_time) {
      return grpc::Status(grpc::StatusCode::NOT_FOUND,
                          "Could not find key to remove.");
    }

    bool exists_key = false;
    uint8_t del_elt_pos = current_array.GetPosition(key, &exists_key);
    if (!exists_key) {
      return grpc::Status(grpc::StatusCode::NOT_FOUND,
                          "Could not find key to remove.");
    }

    FlatMap<EltType> new_version(current_array, del_elt_pos);
    return elements_->Set(new_version, tx, edit_time);
  }

  inline grpc::Status Remove(const std::string& key, const Transaction* tx) {
    return Remove(key.c_str(), tx);
  }

  WriteTransaction::OpFunction RemoveCallback(const char* key,
                                              uint64_t edit_time) {
    grpc::Status (TxAwareFlatMap::*fn)(
        const char*, const Transaction*, uint64_t) = &TxAwareFlatMap::Remove;
    return std::bind(fn, this, key, std::placeholders::_1, edit_time);
  }

  grpc::Status Remove(const char* key, uint32_t value, const Transaction* tx,
                      uint64_t edit_time) {
    if (!allow_dups_) {
      return Remove(key, tx, edit_time);
    }
    acumio::transaction::ExclusiveLock guard(guard_);
    grpc::Status check = VerifyTxEditTime(tx, edit_time);
    if (!check.ok()) {
      return check;
    }
    bool exists_at_time = false;
    const FlatMap<EltType>& current_array =
        elements_->Get(edit_time, &exists_at_time);
    if (!exists_at_time) {
      return grpc::Status(grpc::StatusCode::NOT_FOUND,
                          "Could not find key to remove.");
    }

    bool exists_key = false;
    uint8_t del_elt_pos = current_array.GetPosition(key, value, &exists_key);
    if (!exists_key) {
      return grpc::Status(grpc::StatusCode::NOT_FOUND,
                          "Could not find key to remove.");
    }

    FlatMap<EltType> new_version(current_array, del_elt_pos);
    return elements_->Set(new_version, tx, edit_time);
  }

  inline grpc::Status Remove(const std::string& key, uint32_t value,
                             const Transaction* tx) {
    return Remove(key.c_str(), value, tx);
  }

  WriteTransaction::OpFunction RemoveCallback(const char* key, uint32_t value,
                                              uint64_t edit_time) {
    grpc::Status (TxAwareFlatMap::*fn)(const char*, uint32_t,
        const Transaction*, uint64_t) = &TxAwareFlatMap::Remove;
    return std::bind(fn, this, key, value, std::placeholders::_1, edit_time);
  }

  grpc::Status Get(const char* key, uint32_t* value,
                   uint64_t access_time) const {
    acumio::transaction::SharedLock guard(guard_);
    bool found = false;
    const FlatMap<EltType>& array = elements_->Get(access_time, &found);
    if (!found) {
      return grpc::Status(grpc::StatusCode::NOT_FOUND,
                          "Could not find key to remove.");
    }
    *value = array.GetValuePosition(key, &found);
    if (!found) {
      return grpc::Status(grpc::StatusCode::NOT_FOUND,
                          "Could not find key to remove.");
    }
    
    return grpc::Status::OK;
  }

  inline grpc::Status Get(const std::string& key, uint32_t* value,
                          uint64_t access_time) const {
    return Get(key.c_str(), value, access_time);
  }

  void CompleteWriteOperation(const Transaction* tx) {
    acumio::transaction::ExclusiveLock guard(guard_);
    elements_->CompleteWrite(tx);
  }

  WriteTransaction::CompletionFunction CompleteWriteCallback() {
    void (TxAwareFlatMap::*fn)(const Transaction*) =
        &TxAwareFlatMap::CompleteWriteOperation;
    return std::bind(fn, this, std::placeholders::_1);
  }

  void Rollback(const Transaction* tx) {
    acumio::transaction::ExclusiveLock guard(guard_);
    elements_->Rollback(tx);
  }

  WriteTransaction::RollbackFunction RollbackCallback() {
    void (TxAwareFlatMap::*fn)(const Transaction*) =
        &TxAwareFlatMap::Rollback;
    return std::bind(fn, this, std::placeholders::_1);
  }

  Iterator LowerBound(const char* key, uint64_t tx_time) {
    acumio::transaction::SharedLock guard(guard_);
    bool exists = false;
    const FlatMap<EltType>& reference = elements_->Get(tx_time, &exists);
    if (!exists) {
      Iterator ret_val;
      return ret_val;
    }
    bool unused;
    uint8_t key_pos = reference.GetPosition(key, &unused);
    _InnerIter delegate_ret_val(&reference, key);
    return Iterator(delegate_ret_val, &guard_);
  }

  inline Iterator LowerBound(const std::string& key, uint64_t tx_time) {
    return LowerBound(key.c_str(), tx_time);
  }

  const Iterator begin(uint64_t tx_time) const {
    acumio::transaction::SharedLock guard(guard_);
    bool exists = false;
    const FlatMap<EltType>& reference = elements_->Get(tx_time, &exists);
    _InnerIter delegate_ret_val(&reference, 0);
    return Iterator(delegate_ret_val, &guard_);
  }

  const Iterator end(uint64_t tx_time) const {
    acumio::transaction::SharedLock guard(guard_);
    bool exists = false;
    const FlatMap<EltType>& reference = elements_->Get(tx_time, &exists);
    _InnerIter delegate_ret_val(&reference, reference.Size());
    return Iterator(delegate_ret_val, &guard_);
  }

  inline uint16_t max_space() const { return max_space_; }
  inline bool allow_dups() const { return allow_dups_; }

  // Returns the size of map at the time indicated, and assuming the map
  // has a recorded version at that time, exists_at_time is set to true.
  // If there are no versions available at the indicated time, this returns
  // the value 0 and exists_at_time is set to false.
  // Note that accessing historical versions is mutable, since this class
  // cleans up versions that are older than cleanup_nanos_.
  uint8_t size(uint64_t time, bool* exists_at_time) const {
    acumio::transaction::SharedLock guard(guard_);
    const FlatMap<EltType>& reference = elements_->Get(time, exists_at_time);
    return reference.Size();
  }

 private:
  grpc::Status VerifyTxEditTime(const Transaction* tx, uint64_t edit_time) {
    if (edit_time != tx->operation_start_time()) {
      return grpc::Status(grpc::StatusCode::DEADLINE_EXCEEDED,
                          "Transaction timed out.");
    }
    if (!elements_->IsLatestVersionAtTime(edit_time)) {
      return grpc::Status(grpc::StatusCode::ABORTED,
                          "Concurrent edit conflict.");
    }
    return grpc::Status::OK;
  }

  typedef TxAware<FlatMap<EltType>> VersionArray;

  uint64_t cleanup_nanos_;
  uint16_t max_space_;
  uint8_t max_size_;
  bool allow_dups_;
  mutable SharedMutex guard_;
  // Note that we don't version our keys directly. If a node changes a key,
  // we will simply refer to both the old and new versions.
  std::unique_ptr<StringAllocator> key_allocator_;
  ObjectAllocator<EltType>* object_allocator_;

  VersionArray* elements_;
}; // end class TxAwareFlatMap

} // namespace collection
} // namespace acumio
#endif // AcumioServer_tx_aware_flat_map_h
