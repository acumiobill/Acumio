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
//               the constructor parameter "allow_duplicates". However, this
//               only defines duplication with respect to keys. So, if
//               allow_duplicates is set to true, we will allow for the same
//               key to reference multiple different values. However, even in
//               this casse, we reject having the same key-value pair
//               represented multiple times.
//============================================================================

//#include <atomic>
#include <functional>
#include <grpc++/support/status.h>
#include <iterator>
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include "flat_map.h"
#include "iterators.h"
#include "tx_managed_map.h"
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

template <typename EltType>
class TxAwareFlatMap : public TxManagedMap<EltType> {
 public:
  // The C++ compiler gets a bit confused about how to lookup the function
  // call for allow_duplicates() since it is present in the base class.
  // With this "using" statement, the compiler gives an error when trying
  // to invoke the function. An alternative is to invoke allow_duplicates()
  // using the syntax this->allow_duplicates() instead, but this will be
  // equally if not more confusing.
  using UnadaptedTxManagedMap::allow_duplicates;

  typedef typename FlatMap<EltType>::Iterator _InnerIter;

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
  // The allow_duplicates parameter determines if duplicate keys are accepted
  // or rejected. Having said that, having a duplicate of both key and value is
  // *never* allowed.
  TxAwareFlatMap(ObjectAllocator<EltType>* object_allocator,
                 uint16_t max_key_space, uint8_t max_size,
                 uint64_t create_time, bool allow_duplicates = false) :
      TxManagedMap<EltType>(object_allocator, allow_duplicates),
      max_space_(max_key_space),
      max_size_(max_size),
      key_allocator_(new StringAllocator(max_key_space)),
      elements_(nullptr) {
    FlatMap<EltType> not_present_value;
    elements_ = new VersionArray(not_present_value, create_time);
  }

  // A FlatMap in this form is not actually usable. However, we have this
  // so we could create arrays of TxAwareFlatMap objects.
  TxAwareFlatMap() : TxAwareFlatMap(nullptr, 0, 0, 0, false) {}

  ~TxAwareFlatMap() {
    delete elements_;
  }

  grpc::Status Add(const char* key, uint32_t value_position,
                   const Transaction* tx, uint64_t tx_time) {
    acumio::transaction::ExclusiveLock guard(guard_);
    grpc::Status check = VerifyTxEditTime(tx, tx_time);
    if (!check.ok()) {
      return check;
    }
    bool exists_at_time = false;
    const FlatMap<EltType>& current_array =
        elements_->Get(tx_time, &exists_at_time);
    if (exists_at_time && current_array.size() == max_size_) {
      // TODO: Migrate this to using an enum code that allows us to detect
      //       that this is a likely condition where we should burst.
      return grpc::Status(grpc::StatusCode::OUT_OF_RANGE,
                          "FlatMap has reached max size.");
    }

    bool exists_key = false;
    uint8_t new_elt_pos = (
        allow_duplicates() ?
        current_array.GetInternalPosition(key, value_position, &exists_key) :
        current_array.GetInternalPosition(key, &exists_key));
    if (exists_key) {
      return grpc::Status(grpc::StatusCode::ALREADY_EXISTS,
                          "There is already an entry with the given key.");
    }

    uint16_t key_pos = key_allocator_->Add(key);
    if (key_pos == max_size_) {
      // TODO: Migrate this to using an enum code that allows us to detect
      //       that this is a likely condition where we should burst.
      return grpc::Status(grpc::StatusCode::OUT_OF_RANGE,
                          "FlatMap has reached max key space.");
    }

    FlatMap<EltType> new_version(current_array, key_pos, value_position,
                                 new_elt_pos);
    // The key_allocator_->Add(key) and the use of the above constructor
    // would double-count the addition of the key. For this reason, we
    // drop one of the references.
    key_allocator_->DropReference(key_pos);
    grpc::Status result = elements_->Set(new_version, tx, tx_time);
    if (!result.ok()) {
      key_allocator_->DropReference(key_pos);
    }
    return result;
  }
 
  grpc::Status Replace(const char* key, uint32_t value, const Transaction* tx,
                       uint64_t tx_time) {
    acumio::transaction::ExclusiveLock guard(guard_);
    grpc::Status check = VerifyTxEditTime(tx, tx_time);
    if (!check.ok()) {
      return check;
    }
    bool exists_at_time = false;
    const FlatMap<EltType>& current_array =
        elements_->Get(tx_time, &exists_at_time);
    if (!exists_at_time) {
      return grpc::Status(grpc::StatusCode::NOT_FOUND,
                          "Could not find key to replace.");
    }

    bool exists_key = false;
    uint8_t new_elt_pos = (allow_duplicates() ?
                           current_array.GetInternalPosition(key, value,
                                                             &exists_key) :
                           current_array.GetInternalPosition(key, &exists_key));
    if (!exists_key) {
      return grpc::Status(grpc::StatusCode::NOT_FOUND,
                          "Could not find key to replace.");
    }

    FlatMap<EltType> update_array(current_array);
    update_array.PutUsingArrayPosition(new_elt_pos, value);

    return elements_->Set(update_array, tx, tx_time);
  }

  grpc::Status Remove(const char* key, const Transaction* tx,
                      uint64_t tx_time) {
    if (allow_duplicates()) {
      return grpc::Status(
          grpc::StatusCode::FAILED_PRECONDITION,
          "This method is not valid if duplicate keys are allowed.");
    }
    acumio::transaction::ExclusiveLock guard(guard_);
    grpc::Status check = VerifyTxEditTime(tx, tx_time);
    if (!check.ok()) {
      return check;
    }
    bool exists_at_time = false;
    const FlatMap<EltType>& current_array =
        elements_->Get(tx_time, &exists_at_time);
    if (!exists_at_time) {
      return grpc::Status(grpc::StatusCode::NOT_FOUND,
                          "Could not find key to remove.");
    }

    bool exists_key = false;
    uint8_t del_elt_pos = current_array.GetInternalPosition(key, &exists_key);
    if (!exists_key) {
      return grpc::Status(grpc::StatusCode::NOT_FOUND,
                          "Could not find key to remove.");
    }

    FlatMap<EltType> new_version(current_array, del_elt_pos);
    return elements_->Set(new_version, tx, tx_time);
  }

  grpc::Status Remove(const char* key, uint32_t value, const Transaction* tx,
                      uint64_t tx_time) {
    if (!(allow_duplicates())) {
      return Remove(key, tx, tx_time);
    }
    acumio::transaction::ExclusiveLock guard(guard_);
    grpc::Status check = VerifyTxEditTime(tx, tx_time);
    if (!check.ok()) {
      return check;
    }
    bool exists_at_time = false;
    const FlatMap<EltType>& current_array =
        elements_->Get(tx_time, &exists_at_time);
    if (!exists_at_time) {
      return grpc::Status(grpc::StatusCode::NOT_FOUND,
                          "Could not find key to remove.");
    }

    bool exists_key = false;
    uint8_t del_elt_pos = current_array.GetInternalPosition(key, value,
                                                            &exists_key);
    if (!exists_key) {
      return grpc::Status(grpc::StatusCode::NOT_FOUND,
                          "Could not find key to remove.");
    }

    FlatMap<EltType> new_version(current_array, del_elt_pos);
    return elements_->Set(new_version, tx, tx_time);
  }

  grpc::Status GetValuePosition(const char* key, uint32_t* value,
                                uint64_t access_time) const {
    acumio::transaction::SharedLock guard(guard_);
    bool found = false;
    const FlatMap<EltType>& array = elements_->Get(access_time, &found);
    if (!found) {
      return grpc::Status(grpc::StatusCode::NOT_FOUND,
                          "Could not find key.");
    }
    *value = array.GetValuePosition(key, &found);
    if (!found) {
      return grpc::Status(grpc::StatusCode::NOT_FOUND,
                          "Could not find key.");
    }
    
    return grpc::Status::OK;
  }

  virtual void CleanVersions(uint64_t clean_time) {
    acumio::transaction::ExclusiveLock guard(guard_);
    elements_->CleanVersions(clean_time);
  }

  void CompleteWriteOperation(const Transaction* tx) {
    acumio::transaction::ExclusiveLock guard(guard_);
    elements_->CompleteWrite(tx);
  }

  void Rollback(const Transaction* tx) {
    acumio::transaction::ExclusiveLock guard(guard_);
    elements_->Rollback(tx);
  }

  std::unique_ptr<TxBasicIterator> LowerBound(const char* key,
                                              uint64_t access_time) const {
    acumio::transaction::SharedLock guard(guard_);
    bool exists = false;
    const FlatMap<EltType>& reference = elements_->Get(access_time, &exists);
    uint8_t key_pos = exists ? reference.GetInternalPosition(key, &exists)
                             : reference.size();
    std::shared_ptr<TxBasicIterator> delegate_ret_val(
        new _InnerIter(&reference, key_pos));
    std::unique_ptr<TxBasicIterator> ret_val(
        new TxManagedIterator(delegate_ret_val, &guard_));
    return ret_val;
  }

  std::unique_ptr<TxBasicIterator> Begin(uint64_t access_time) const {
    acumio::transaction::SharedLock guard(guard_);
    bool exists = false;
    const FlatMap<EltType>& reference = elements_->Get(access_time, &exists);
    std::shared_ptr<TxBasicIterator> delegate_ret_val(
        new _InnerIter(&reference, 0));
    std::unique_ptr<TxBasicIterator> ret_val(
        new TxManagedIterator(delegate_ret_val, &guard_));
    return ret_val;
  }

  std::unique_ptr<TxBasicIterator> ReverseBegin(uint64_t access_time) const {
    acumio::transaction::SharedLock guard(guard_);
    bool exists = false;
    const FlatMap<EltType>& reference = elements_->Get(access_time, &exists);
    std::shared_ptr<TxBasicIterator> delegate_ret_val(
        new _InnerIter(&reference,
                       reference.size() == 0 ? 0 : reference.size() - 1));
    std::unique_ptr<TxBasicIterator> ret_val(
        new TxManagedIterator(delegate_ret_val, &guard_));
    return ret_val;
  }

  std::unique_ptr<TxBasicIterator> End(uint64_t access_time) const {
    acumio::transaction::SharedLock guard(guard_);
    bool exists = false;
    const FlatMap<EltType>& reference = elements_->Get(access_time, &exists);
    std::shared_ptr<TxBasicIterator> delegate_ret_val(
        new _InnerIter(&reference, reference.size()));
    std::unique_ptr<TxBasicIterator> ret_val(
        new TxManagedIterator(delegate_ret_val, &guard_));
    return ret_val;
  }

  inline uint16_t max_space() const { return max_space_; }

  // Returns the size of map at the time indicated, and assuming the map
  // has a recorded version at that time, exists_at_time is set to true.
  // If there are no versions available at the indicated time, this returns
  // the value 0 and exists_at_time is set to false.
  // The return-value here is a uint32_t to match the TxManagedMap, but
  // in reality, the value will be limited to a uint8_t.
  uint32_t Size(uint64_t access_time, bool* exists_at_time) const {
    acumio::transaction::SharedLock guard(guard_);
    const FlatMap<EltType>& reference = elements_->Get(access_time,
                                                       exists_at_time);
    return reference.size();
  }

 private:
  grpc::Status VerifyTxEditTime(const Transaction* tx, uint64_t tx_time) {
    if (tx_time != tx->operation_start_time()) {
      return grpc::Status(grpc::StatusCode::DEADLINE_EXCEEDED,
                          "Transaction timed out.");
    }
    if (!elements_->IsLatestVersionAtTime(tx_time)) {
      return grpc::Status(grpc::StatusCode::ABORTED,
                          "Concurrent edit conflict.");
    }
    return grpc::Status::OK;
  }

  typedef TxAware<FlatMap<EltType>> VersionArray;

  uint16_t max_space_;
  uint8_t max_size_;
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
