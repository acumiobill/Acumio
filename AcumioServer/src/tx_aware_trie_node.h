#ifndef AcumioServer_tx_aware_trie_node_h
#define AcumioServer_tx_aware_trie_node_h
//============================================================================
// Name        : tx_aware_trie_node.h
// Author      : Bill Province (bill@acumio.com)
// Version     :
// Copyright   : Copyright (C) 2016 Acumio
// Description : This file contains the definition for a TxAwareTrieNode.
//               This is a critical comp of the TxAwareBurstTrie data
//               structure, which is based in part on "HAT-trie: A
//               Cache-conscious Trie-based Data Structure for Strings"
//               (Nikolas Askitis, Ranjan Sinha).
//
//               See tx_aware_burst_trie.h for more details.
//============================================================================

#include <grpc++/support/status.h>
#include <memory>
#include <string>
#include "flat_set.h"
#include "object_allocator.h"
#include "shared_mutex.h"
#include "tx_managed_map.h"

namespace acumio {
namespace collection {

using acumio::transaction::ExclusiveLock;
using acumio::transaction::SharedLock;
using acumio::transaction::SharedMutex;
using acumio::transaction::Transaction;
using acumio::transaction::WriteTransaction;
using acumio::transaction::TxAware;
using acumio::transaction::TxPointerLess;

template <typename EltType>
class TxManagedMapFactory {
 public:
  TxManagedMapFactory(ObjectAllocator<EltType>* object_allocator,
                      bool allow_duplicates = false) :
      object_allocator_(object_allocator),
      allow_duplicates_(allow_duplicates) {}
  TxManagedMapFactory() :  TxManagedMapFactory(nullptr, false) {}
  ~TxManagedMapFactory() {}

  // Caller must assume ownership of generated object.
  virtual TxManagedMap<EltType>* CreateNew(uint64_t create_time) = 0;
  // Caller must assume ownership of generated object.
  virtual TxManagedMap<EltType>* CreateCopy(TxManagedMap<EltType>* other,
                                            uint64_t create_time) = 0;

 private:
  ObjectAllocator<EltType>* object_allocator_;
  bool allow_duplicates_;
};

template <typename EltType>
class TxAwareTrieNode : public TxManagedMap<EltType> {
 public:

  friend class Iterator;
  typedef FlatSet<uint8_t>::const_iterator FlatIterator;

  class Iterator : public TxBasicIterator {
   public:
    // While these constructors are public for use in the TxAwareTrieNode,
    // they really should only be available within the TxAwareTrieNode.
    // Public access should be via the TxAwareTrieNode::begin(),
    // TxAwareTrieNode::end(), and TxAwareTrieNdoe::LowerBound(const char*)
    // methods.
    //
    // precondition: The index_delegate and child_delegate should come from
    //               the provided container. The operation is undefined if
    //               either index_delegate or child_delegate are related
    //               to different structures other than the container.
    //
    // precondition:
    //    Assuming the container is not null then:
    //        (index_delegate.get() == nullptr ||
    //         index_delegate.get() == container.populated_.end())
    //        iff
    //        (child_delegate.get() == nullptr)
    //
    // In other words, it makes no sense to use a child_delegate that refers
    // to an actual "something" if the index_delegate is either nullptr or
    // pointing to the end of the iterator.
    //
    // precondition:
    //    Assuming the container is not null and child_delegate is not null
    //    then:
    //        *child_delegate != container->IndexedEnd(**index_delegate,
    //                                                 access_time);
    // In other words, if you pass in a child_delegate, it should always
    // be a dereferencable value. A nullptr is okay however, assuming that
    // you really are at the very end of iteration over all index values.
    Iterator(const TxAwareTrieNode* container,
             std::unique_ptr<FlatIterator> index_delegate,
             std::unique_ptr<TxBasicIterator> child_delegate,
             uint64_t access_time,
             SharedMutex* guard) :
        TxBasicIterator(guard),
        container_(container), index_delegate_(std::move(index_delegate)),
        child_delegate_(std::move(child_delegate)), access_time_(access_time),
        index_end_(nullptr), index_(0), child_end_(nullptr),
        saved_tmp_(nullptr), empty_val_() {
      // See precondition.
      if (container != nullptr) {
        index_end_.reset(new FlatIterator(container->populated_.end()));
        if (index_delegate_.get() == nullptr) {
          index_delegate_.reset(
              new FlatIterator(container->populated_.end()));
        } else if (*index_delegate_ != *index_end_) {
          // assert: child_delegate.get() != nullptr.
          index_ = **index_delegate_;
          child_end_ = container->IndexedEndIterator(index_, access_time_);
        }
      }
    }

    // Same as above constructor but with index_delegate and child_delegate
    // specified as nullptr.
    Iterator(const TxAwareTrieNode* container,
             uint64_t access_time, SharedMutex* guard) :
        TxBasicIterator(guard),
        container_(container), index_delegate_(nullptr),
        child_delegate_(nullptr), access_time_(access_time),
        index_end_(nullptr), index_(0), child_end_(nullptr),
        saved_tmp_(nullptr), empty_val_() {
      if (container != nullptr) {
        index_delegate_.reset(new FlatIterator(container->populated_.end()));
        index_end_.reset(new FlatIterator(container->populated_.end()));
      }
    }

    Iterator(const Iterator& other) :
        TxBasicIterator(other.guard()),
        container_(other.container_), index_delegate_(nullptr),
        child_delegate_(nullptr), access_time_(other.access_time_),
        index_end_(nullptr), index_(other.index_), child_end_(nullptr),
        saved_tmp_(nullptr), empty_val_() {
      if (other.index_delegate_.get() != nullptr) {
        index_delegate_.reset(new FlatIterator(*(other.index_delegate_)));
      }
      if (other.child_delegate_.get() != nullptr) {
        child_delegate_.reset(other.child_delegate_->Clone());
      }
      if (other.index_end_.get() != nullptr) {
        index_end_.reset(new FlatIterator(*(other.index_end_)));
      }
      if (other.child_end_.get() != nullptr) {
        child_end_.reset(other.child_end_->Clone());
      }
    }

    Iterator() : Iterator(nullptr, UINT64_C(0), nullptr) {}

    TxBasicIterator* Clone() const { return new Iterator(*this); }

    // pre-increment/decrement
    TxBasicIterator& operator++() {
      if (child_delegate_.get() == nullptr) {
        return *this;
      }

      (*child_delegate_)++;
      while ((child_delegate_.get() == nullptr ||
              *child_delegate_ == *child_end_) &&
             *index_delegate_ != *index_end_) {
        (*index_delegate_)++;
        if (*index_delegate_ != *index_end_) {
          index_ = **index_delegate_;
          std::unique_ptr<TxBasicIterator> new_child =
              container_->IndexedBeginIterator(index_, access_time_);
          if (new_child.get() == nullptr) {
            child_delegate_.reset(nullptr);
          } else {
            child_delegate_.reset(new_child.release());
            std::unique_ptr<TxBasicIterator> new_child_end =
                container_->IndexedEndIterator(index_, access_time_);
            child_end_.reset(new_child_end.release());
          }
        }
      }

      if (*index_delegate_ == *index_end_) {
        child_delegate_.reset(nullptr);
        child_end_.reset(nullptr);
      }

      return *this;
    }

    TxBasicIterator& operator--() {
      if (child_delegate_.get() == nullptr) {
        return *this;
      }

      (*child_delegate_)--;
      while ((child_delegate_.get() == nullptr ||
              *child_delegate_ == *child_end_) &&
             *index_delegate_ != *index_end_) {
        (*index_delegate_)--;
        if (*index_delegate_ != *index_end_) {
          index_ = **index_delegate_;
          std::unique_ptr<TxBasicIterator> new_child =
              container_->IndexedReverseBeginIterator(index_, access_time_);
          if (new_child.get() == nullptr) {
            child_delegate_.reset(nullptr);
          } else {
            child_delegate_.reset(new_child.release());
            std::unique_ptr<TxBasicIterator> new_child_end =
                container_->IndexedEndIterator(index_, access_time_);
            child_end_.reset(new_child_end.release());
          }
        }
      }

      if (*index_delegate_ == *index_end_) {
        child_delegate_.reset(nullptr);
        child_end_.reset(nullptr);
      }

      return *this;
    }

    // post-increment/decrement
    TxBasicIterator& operator++(int) {
      saved_tmp_.reset(new Iterator(*this));
      this->operator++();
      return *saved_tmp_;
    }

    TxBasicIterator& operator--(int) {
      saved_tmp_.reset(new Iterator(*this));
      this->operator--();
      return *saved_tmp_;
    }

    bool operator==(const TxBasicIterator& other) const {
      const Iterator* other_iter = dynamic_cast<const Iterator*>(&other);
      if (other_iter == nullptr) {
        return false;
      }
      if (other_iter->container_ != container_) {
        return false;
      }

      if (child_delegate_.get() == nullptr) {
        return other_iter->child_delegate_.get() == nullptr;
      }

      if (index_ != other_iter->index_) {
        return false;
      }

      // We skip comparing with index_delegate_, since the comparison
      // of the children will serve the same purpose. While comparing
      // the child delegates is potentially more expensive, the quick
      // check comparing index_ values serves to short-circuit the
      // comparison if the index_delegate_ values are not the same.
      return (*child_delegate_ == *(other_iter->child_delegate_));
    }

    const MapIterElement& operator*() const {
      if (child_delegate_.get() != nullptr) {
        const MapIterElement& child_elt = **child_delegate_;
        RopePiece elt_key(new RopePiece(static_cast<char>(index_)),
                          new RopePiece(child_elt.key));
        saved_val_.reset(new MapIterElement(elt_key, child_elt.value));
        return *saved_val_;
      }
      return empty_val_;
    }

    const MapIterElement* operator->() const {
      if (child_delegate_.get() != nullptr) {
        const MapIterElement& child_elt = **child_delegate_;
        RopePiece elt_key(new RopePiece(static_cast<char>(index_)),
                          new RopePiece(child_elt.key));
        saved_val_.reset(new MapIterElement(elt_key, child_elt.value));
        return saved_val_.get();
      }
      return &empty_val_;
    }

   private:
    const TxAwareTrieNode* container_;
    std::unique_ptr<FlatIterator> index_delegate_;
    std::unique_ptr<TxBasicIterator> child_delegate_;
    uint64_t access_time_;
    std::unique_ptr<FlatIterator> index_end_;
    uint8_t index_;
    std::unique_ptr<TxBasicIterator> child_end_;
    std::unique_ptr<Iterator> saved_tmp_;
    mutable std::unique_ptr<MapIterElement> saved_val_;
    MapIterElement empty_val_;
  };

  TxAwareTrieNode(TxManagedMapFactory<EltType>* leaf_factory,
                  TxManagedMapFactory<EltType>* intermediate_factory,
                  ObjectAllocator<EltType>* object_allocator,
                  bool allow_duplicates = false) :
      TxManagedMap<EltType>(object_allocator, allow_duplicates),
      guard_(),
      leaf_factory_(leaf_factory),
      intermediate_factory_(intermediate_factory),
      populated_(256),
      tx_edits_map_() {}

  TxAwareTrieNode() :
      TxAwareTrieNode<EltType>(nullptr, nullptr, nullptr, false) {}

  ~TxAwareTrieNode() {}

  grpc::Status GetValuePosition(const char* key, uint32_t* value,
                                uint64_t access_time) const {
    if (key == nullptr) {
      return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT,
                          "key must not be null.");
    }
    if (value == nullptr) {
      return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT,
                          "value pointer must not be null.");
    }

    uint8_t index = IndexFromKey(key);
    SharedLock read_lock(guard_);

    const TxManagedMap<EltType>* map = MapVersionAtTime(index, access_time);
    if (map == nullptr) {
      return grpc::Status(grpc::StatusCode::NOT_FOUND,
                          "Could not find key at given time.");
    }

    return map->GetValuePosition(key + 1, value, access_time);
  }

  uint32_t Size(uint64_t access_time, bool* exists_at_time) const {
    FlatIterator begin = populated_.begin();
    FlatIterator end = populated_.end();
    FlatIterator it = begin;
    uint32_t ret_val = 0;
    *exists_at_time = false;
    for (; it != end && !(*exists_at_time); it++) {
      uint8_t index = *it;
      const TxManagedMap<EltType>* map = MapVersionAtTime(index, access_time);
      if (map != nullptr) {
        ret_val += map->Size(access_time, exists_at_time);
      }
    }

    for (; it != end; it++) {
      uint8_t index = *it;
      bool unused = false;
      const TxManagedMap<EltType>* map = MapVersionAtTime(index, access_time);
      if (map != nullptr) {
        ret_val += map->Size(access_time, &unused);
      }
    }

    return ret_val;
  }

  grpc::Status Add(const char* key, uint32_t value, const Transaction* tx,
                   uint64_t tx_time) {
    if (key == nullptr || key[0] == '\0') {
      return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT,
                          "key must not be null and have length > 0.");
    }

    uint8_t index = IndexFromKey(key);

    acumio::transaction::ExclusiveLock lock(guard_);
    // First, we verify Transaction info.
    Transaction::AtomicInfo current_tx_info = tx->GetAtomicInfo();
    if (current_tx_info.operation_start_time != tx_time) {
      return grpc::Status(grpc::StatusCode::DEADLINE_EXCEEDED,
        "The transaction timed out before completion.");
    }

    EditInfo& edit_info = edits_[index];
    MapVersion& current_version = current_values_[index];
    grpc::Status ret_val = VerifyNoConflictingEdits(tx, edit_info,
                                                    current_version, tx_time);

    if (!ret_val.ok()) {
      return ret_val;
    }

    // assert: edit_info.state == NOT_EDITING ||
    //         edit_info.state == PASS_THROUGH
    // The other two cases would have generated an error during
    // VerifyNoConflictingEdits.
    if (edit_info.state == PASS_THROUGH) {
      ret_val = current_version.value->Add(key + 1, value, tx, tx_time);
      if (ret_val.ok()) {
        edit_info.edit_transactions.insert(tx);
        return grpc::Status::OK;
      }

      // TODO: Migrate Error Codes of content to detect when a possible
      //       burst condition is a likely valid response. Using
      //       grpc's Status code is a bit limiting. We are checking two
      //       things here: first that we have a possible "burst" condition
      //       according to our current value and second, that we are in
      //       a configuration where our current value is a leaf-level
      //       map and our historical value is empty. Only in this case
      //       can we burst.
      if (ret_val.error_code() != grpc::StatusCode::OUT_OF_RANGE ||
          historical_values_[index].value != nullptr) {
        return ret_val;
      }

      // If we have reached here, we should burst the current value
      // and create a new edit version with the burst value. However,
      // if we have more pending transactions on the current value,
      // we should just return a concurrency exception.
      // Notice that when we initially check for a possible conflict with an
      // existing transaction, we might not have noticed this condition,
      // because we only noticed the need to burst the node after we did
      // the initial conflict-check. It's the need to burst the node that
      // causes us to require that there are not conflicting transactions.
      if (edit_info.edit_transactions.size() != 0) {
        return grpc::Status(grpc::StatusCode::ABORTED,
                            "concurrency exception.");
      }

      return BurstAndAdd(key + 1, value, edit_info, current_version, tx,
                         tx_time);
    }

    // assert: edit_info.state == NOT_EDITING.
    if (current_version.value == nullptr) {
      // assert: current_version.times.create == 0,
      //         current_version.times.remove == end_of_time,
      //         historical_version.times.create == 0,
      //         historical_version.times.remove == 0,
      //         edit_info.edit_transactions.size() == 0
      edit_info.edit_value = leaf_factory_->CreateNew(tx_time);
      ret_val = edit_info.edit_value->Add(key + 1, value, tx, tx_time);
      if (!ret_val.ok()) {
        delete edit_info.edit_value;
        edit_info.edit_value = nullptr;
        return ret_val;
      }
      edit_info.state = CREATING;
      edit_info.time = tx_time;
      edit_info.edit_transactions.insert(tx);
      return grpc::Status::OK;
    }

    // assert: edit_info.state == NOT_EDITING and
    //         current_version.value != nullptr
    ret_val = current_version.value->Add(key + 1, value, tx, tx_time);

    if (ret_val.ok()) {
      edit_info.state = PASS_THROUGH;
      edit_info.time = tx_time;
      edit_info.edit_transactions.insert(tx);
      return grpc::Status::OK;
    }

    if (ret_val.error_code() != grpc::StatusCode::OUT_OF_RANGE ||
        historical_values_[index].value != nullptr) {
      return ret_val;
    }

    // Here, we know that historical_values_[index].value was a nullptr,
    // and we got an OUT_OF_RANGE error from trying to add the new element.
    // This is exactly the condition needed to burst the current
    // version.
    return BurstAndAdd(key + 1, value, edit_info, current_version, tx, tx_time);
  }

  // TODO: Consider unifying both Remove methods and the Replace method using
  //       callback functions. All three methods have identical structure
  //       except for calling either Remove/Replace or BurstAndRemove/Replace
  //       internally.
  grpc::Status Remove(const char* key, const Transaction* tx,
                      uint64_t tx_time) {
    if (key == nullptr || key[0] == '\0') {
      return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT,
                          "key must not be null and have length > 0.");
    }

    uint8_t index = IndexFromKey(key);

    acumio::transaction::ExclusiveLock lock(guard_);
    // First, we verify Transaction info.
    Transaction::AtomicInfo current_tx_info = tx->GetAtomicInfo();
    if (current_tx_info.operation_start_time != tx_time) {
      return grpc::Status(grpc::StatusCode::DEADLINE_EXCEEDED,
        "The transaction timed out before completion.");
    }

    EditInfo& edit_info = edits_[index];
    MapVersion& current_version = current_values_[index];
    grpc::Status ret_val = VerifyNoConflictingEdits(tx, edit_info,
                                                    current_version, tx_time);

    if (!ret_val.ok()) {
      return ret_val;
    }

    // assert: edit_info.state == NOT_EDITING ||
    //         edit_info.state == PASS_THROUGH
    // The other two cases would have generated an error during
    // VerifyNoConflictingEdits.
    if (edit_info.state == PASS_THROUGH) {
      ret_val = current_version.value->Remove(key + 1, tx, tx_time);
      if (ret_val.ok()) {
        edit_info.edit_transactions.insert(tx);
        return grpc::Status::OK;
      }

      // TODO: Migrate Error Codes of content to detect when a possible
      //       burst condition is a likely valid response. Using
      //       grpc's Status code is a bit limiting. We are checking two
      //       things here: first that we have a possible "burst" condition
      //       according to our current value and second, that we are in
      //       a configuration where our current value is a leaf-level
      //       map and our historical value is empty. Only in this case
      //       can we burst.
      // Note: Yes, we can end up in a burst condition even when removing
      //       a value, because we cause a new version to be generated.
      if (ret_val.error_code() != grpc::StatusCode::OUT_OF_RANGE ||
          historical_values_[index].value != nullptr) {
        return ret_val;
      }

      // If we have reached here, we should burst the current value
      // and create a new edit version with the burst value. However,
      // if we have more pending transactions on the current value,
      // we should just return a concurrency exception.
      // Notice that when we initially check for a possible conflict with an
      // existing transaction, we might not have noticed this condition,
      // because we only noticed the need to burst the node after we did
      // the initial conflict-check. It's the need to burst the node that
      // causes us to require that there are not conflicting transactions.
      if (edit_info.edit_transactions.size() != 0) {
        return grpc::Status(grpc::StatusCode::ABORTED,
                            "concurrency exception.");
      }

      return BurstAndRemove(key + 1, edit_info, current_version, tx, tx_time);
    }

    // assert: edit_info.state == NOT_EDITING.
    if (current_version.value == nullptr) {
      return grpc::Status(grpc::StatusCode::NOT_FOUND,
                          "unable to find key to remove.");
    }

    // assert: edit_info.state == NOT_EDITING and
    //         current_version.value != nullptr
    ret_val = current_version.value->Remove(key + 1, tx, tx_time);

    if (ret_val.ok()) {
      edit_info.state = PASS_THROUGH;
      edit_info.time = tx_time;
      edit_info.edit_transactions.insert(tx);
      return grpc::Status::OK;
    }

    if (ret_val.error_code() != grpc::StatusCode::OUT_OF_RANGE ||
        historical_values_[index].value != nullptr) {
      return ret_val;
    }

    // Here, we know that historical_values_[index].value was a nullptr,
    // and we got an OUT_OF_RANGE error from trying to add the new element.
    // This is exactly the condition needed to burst the current
    // version.
    return BurstAndRemove(key + 1, edit_info, current_version, tx, tx_time);
  }

  grpc::Status Remove(const char* key, uint32_t value, const Transaction* tx,
                      uint64_t tx_time) {
    if (key == nullptr) {
      return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT,
                          "key must not be null.");
    }

    uint8_t index = IndexFromKey(key);

    acumio::transaction::ExclusiveLock lock(guard_);
    // First, we verify Transaction info.
    Transaction::AtomicInfo current_tx_info = tx->GetAtomicInfo();
    if (current_tx_info.operation_start_time != tx_time) {
      return grpc::Status(grpc::StatusCode::DEADLINE_EXCEEDED,
        "The transaction timed out before completion.");
    }

    EditInfo& edit_info = edits_[index];
    MapVersion& current_version = current_values_[index];
    grpc::Status ret_val = VerifyNoConflictingEdits(tx, edit_info,
                                                    current_version, tx_time);

    if (!ret_val.ok()) {
      return ret_val;
    }

    // assert: edit_info.state == NOT_EDITING ||
    //         edit_info.state == PASS_THROUGH
    // The other two cases would have generated an error during
    // VerifyNoConflictingEdits.
    if (edit_info.state == PASS_THROUGH) {
      ret_val = current_version.value->Remove(key + 1, value, tx, tx_time);
      if (ret_val.ok()) {
        edit_info.edit_transactions.insert(tx);
        return grpc::Status::OK;
      }

      // TODO: Migrate Error Codes of content to detect when a possible
      //       burst condition is a likely valid response. Using
      //       grpc's Status code is a bit limiting. We are checking two
      //       things here: first that we have a possible "burst" condition
      //       according to our current value and second, that we are in
      //       a configuration where our current value is a leaf-level
      //       map and our historical value is empty. Only in this case
      //       can we burst.
      // Note: Yes, we can end up in a burst condition even when removing
      //       a value, because we cause a new version to be generated.
      if (ret_val.error_code() != grpc::StatusCode::OUT_OF_RANGE ||
          historical_values_[index].value != nullptr) {
        return ret_val;
      }

      // If we have reached here, we should burst the current value
      // and create a new edit version with the burst value. However,
      // if we have more pending transactions on the current value,
      // we should just return a concurrency exception.
      // Notice that when we initially check for a possible conflict with an
      // existing transaction, we might not have noticed this condition,
      // because we only noticed the need to burst the node after we did
      // the initial conflict-check. It's the need to burst the node that
      // causes us to require that there are not conflicting transactions.
      if (edit_info.edit_transactions.size() != 0) {
        return grpc::Status(grpc::StatusCode::ABORTED,
                            "concurrency exception.");
      }

      return BurstAndRemove(key + 1, value, edit_info, current_version, tx,
                            tx_time);
    }

    // assert: edit_info.state == NOT_EDITING.
    if (current_version.value == nullptr) {
      return grpc::Status(grpc::StatusCode::NOT_FOUND,
                          "unable to find key to remove.");
    }

    // assert: edit_info.state == NOT_EDITING and
    //         current_version.value != nullptr
    ret_val = current_version.value->Remove(key + 1, value, tx, tx_time);

    if (ret_val.ok()) {
      edit_info.state = PASS_THROUGH;
      edit_info.time = tx_time;
      edit_info.edit_transactions.insert(tx);
      return grpc::Status::OK;
    }

    if (ret_val.error_code() != grpc::StatusCode::OUT_OF_RANGE ||
        historical_values_[index].value != nullptr) {
      return ret_val;
    }

    // Here, we know that historical_values_[index].value was a nullptr,
    // and we got an OUT_OF_RANGE error from trying to add the new element.
    // This is exactly the condition needed to burst the current
    // version.
    return BurstAndRemove(key + 1, value, edit_info, current_version, tx,
                          tx_time);
  }

  grpc::Status Replace(const char* key, uint32_t value, const Transaction* tx,
                       uint64_t tx_time) {
    if (key == nullptr || key[0] == '\0') {
      return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT,
                          "key must not be null and have length > 0.");
    }

    uint8_t index = IndexFromKey(key);

    acumio::transaction::ExclusiveLock lock(guard_);
    // First, we verify Transaction info.
    Transaction::AtomicInfo current_tx_info = tx->GetAtomicInfo();
    if (current_tx_info.operation_start_time != tx_time) {
      return grpc::Status(grpc::StatusCode::DEADLINE_EXCEEDED,
        "The transaction timed out before completion.");
    }

    EditInfo& edit_info = edits_[index];
    MapVersion& current_version = current_values_[index];
    grpc::Status ret_val = VerifyNoConflictingEdits(tx, edit_info,
                                                    current_version, tx_time);

    if (!ret_val.ok()) {
      return ret_val;
    }

    // assert: edit_info.state == NOT_EDITING ||
    //         edit_info.state == PASS_THROUGH
    // The other two cases would have generated an error during
    // VerifyNoConflictingEdits.
    if (edit_info.state == PASS_THROUGH) {
      ret_val = current_version.value->Replace(key + 1, value, tx, tx_time);
      if (ret_val.ok()) {
        edit_info.edit_transactions.insert(tx);
        return grpc::Status::OK;
      }

      // TODO: Migrate Error Codes of content to detect when a possible
      //       burst condition is a likely valid response. Using
      //       grpc's Status code is a bit limiting. We are checking two
      //       things here: first that we have a possible "burst" condition
      //       according to our current value and second, that we are in
      //       a configuration where our current value is a leaf-level
      //       map and our historical value is empty. Only in this case
      //       can we burst.
      // Note: Yes, we can end up in a burst condition even when replacing
      //       a value, because we cause a new version to be generated.
      if (ret_val.error_code() != grpc::StatusCode::OUT_OF_RANGE ||
          historical_values_[index].value != nullptr) {
        return ret_val;
      }

      // If we have reached here, we should burst the current value
      // and create a new edit version with the burst value. However,
      // if we have more pending transactions on the current value,
      // we should just return a concurrency exception.
      // Notice that when we initially check for a possible conflict with an
      // existing transaction, we might not have noticed this condition,
      // because we only noticed the need to burst the node after we did
      // the initial conflict-check. It's the need to burst the node that
      // causes us to require that there are not conflicting transactions.
      if (edit_info.edit_transactions.size() != 0) {
        return grpc::Status(grpc::StatusCode::ABORTED,
                            "concurrency exception.");
      }

      return BurstAndReplace(key + 1, value, edit_info, current_version, tx,
                             tx_time);
    }

    // assert: edit_info.state == NOT_EDITING.
    if (current_version.value == nullptr) {
      return grpc::Status(grpc::StatusCode::NOT_FOUND,
                          "unable to find key to remove.");
    }

    // assert: edit_info.state == NOT_EDITING and
    //         current_version.value != nullptr
    ret_val = current_version.value->Replace(key + 1, value, tx, tx_time);

    if (ret_val.ok()) {
      edit_info.state = PASS_THROUGH;
      edit_info.time = tx_time;
      edit_info.edit_transactions.insert(tx);
      return grpc::Status::OK;
    }

    if (ret_val.error_code() != grpc::StatusCode::OUT_OF_RANGE ||
        historical_values_[index].value != nullptr) {
      return ret_val;
    }

    // Here, we know that historical_values_[index].value was a nullptr,
    // and we got an OUT_OF_RANGE error from trying to add the new element.
    // This is exactly the condition needed to burst the current
    // version.
    return BurstAndReplace(key + 1, value, edit_info, current_version, tx,
                           tx_time);
  }

  std::unique_ptr<TxBasicIterator> Begin(uint64_t access_time) const {
    acumio::transaction::SharedLock lock(guard_);

    std::unique_ptr<FlatIterator> index_iter(
        new FlatIterator(populated_.begin()));
    std::unique_ptr<TxBasicIterator> child_iter(nullptr);
    if (*index_iter == populated_.end()) {
      return std::unique_ptr<TxBasicIterator>(
          new Iterator(this, std::move(index_iter), std::move(child_iter),
                       access_time, &guard_));
    }

    child_iter = IndexedBeginIterator(**index_iter, access_time);
    std::unique_ptr<TxBasicIterator> child_end =
        IndexedEndIterator(**index_iter, access_time);
    while ((child_iter.get() == nullptr || *child_iter == *child_end)) {
      index_iter->operator++();
        if (*index_iter == populated_.end()) {
        child_iter.reset(nullptr);
        return std::unique_ptr<TxBasicIterator>(
            new Iterator(this, std::move(index_iter), std::move(child_iter),
                         access_time, &guard_));
      }
      std::unique_ptr<TxBasicIterator> new_child =
          IndexedBeginIterator(**index_iter, access_time);
      child_iter.reset(new_child.release());
      std::unique_ptr<TxBasicIterator> new_end =
          IndexedEndIterator(**index_iter, access_time);
      child_end.reset(new_end.release());
    }

    return std::unique_ptr<TxBasicIterator>(
        new Iterator(this, std::move(index_iter), std::move(child_iter),
                     access_time, &guard_));
  }

  std::unique_ptr<TxBasicIterator> ReverseBegin(uint64_t access_time) const {
    acumio::transaction::SharedLock lock(guard_);

    std::unique_ptr<FlatIterator> index_iter(
        new FlatIterator(populated_.rbegin()));
    std::unique_ptr<TxBasicIterator> child_iter(nullptr);
    if (*index_iter == populated_.end()) {
      return std::unique_ptr<TxBasicIterator>(
          new Iterator(this, std::move(index_iter), std::move(child_iter),
                       access_time, &guard_));
    }

    child_iter = IndexedReverseBeginIterator(**index_iter, access_time);
    std::unique_ptr<TxBasicIterator> child_end =
        IndexedEndIterator(**index_iter, access_time);
    while ((child_iter.get() == nullptr || *child_iter == *child_end)) {
      index_iter->operator--();
        if (*index_iter == populated_.end()) {
        child_iter.reset(nullptr);
        return std::unique_ptr<TxBasicIterator>(
            new Iterator(this, std::move(index_iter), std::move(child_iter),
                         access_time, &guard_));
      }
      std::unique_ptr<TxBasicIterator> new_child =
          IndexedReverseBeginIterator(**index_iter, access_time);
      child_iter.reset(new_child.release());
      std::unique_ptr<TxBasicIterator> new_end =
          IndexedEndIterator(**index_iter, access_time);
      child_end.reset(new_end.release());
    }

    return std::unique_ptr<TxBasicIterator>(
        new Iterator(this, std::move(index_iter), std::move(child_iter),
                     access_time, &guard_));
  }

  std::unique_ptr<TxBasicIterator> End(uint64_t access_time) const {
    return std::unique_ptr<TxBasicIterator>(
        new Iterator(this, access_time, &guard_));
  }

  // Returns a unique_ptr to a nullptr if key == nullptr or if key == "".
  // Empty strings cannot be stored in a TxAwareTrieNode.
  std::unique_ptr<TxBasicIterator> LowerBound(const char* key,
                                              uint64_t access_time) const {
    if (key == nullptr || key[0] == '\0') {
      return std::unique_ptr<TxBasicIterator>(nullptr);
    }

    acumio::transaction::SharedLock lock(guard_);

    uint8_t original_index = IndexFromKey(key);
    std::unique_ptr<FlatIterator> index_iter(
        new FlatIterator(populated_.lower_bound(original_index)));
    FlatIterator end_index_iter = populated_.end();
    if (*index_iter == end_index_iter) {
      return std::unique_ptr<TxBasicIterator>(
          new Iterator(this, access_time, &guard_));
    }

    std::unique_ptr<TxBasicIterator> child_iter(nullptr);
    std::unique_ptr<TxBasicIterator> child_end_iter(nullptr);
    if (**index_iter == original_index) {
      child_iter = (IndexedLowerBoundIterator(key, access_time));
      if (child_iter.get() != nullptr) {
        child_end_iter = IndexedEndIterator(original_index, access_time);
        if (*child_iter != *child_end_iter) {
          return std::unique_ptr<TxBasicIterator>(
              new Iterator(this, std::move(index_iter), std::move(child_iter),
                           access_time, &guard_));
        }
      }
      (*index_iter)++;
    }

    // If we reached here, our returned iterator's key will not match the
    // key of the first character of the requested lower bound key.
    while (*index_iter != end_index_iter) {
      child_iter.reset(
          IndexedBeginIterator(**index_iter, access_time).release());
      if (child_iter.get() != nullptr) {
        child_end_iter.reset(
            IndexedEndIterator(**index_iter, access_time).release());
        if (*child_iter != *child_end_iter) {
          return std::unique_ptr<TxBasicIterator>(
              new Iterator(this, std::move(index_iter), std::move(child_iter),
                           access_time, &guard_));
        }
      }
      (*index_iter)++;
    }

    return std::unique_ptr<TxBasicIterator>(
        new Iterator(this, access_time, &guard_));
  }

  // Removes versions if their time-span does not cover clean_time, and if
  // their time-span comes before clean_time. i.e.:
  // remove if version.times.remove <= clean_time.
  // Skip elements if the time-span in question is currently being edited.
  void CleanVersions(uint64_t clean_time) {
    acumio::transaction::ExclusiveLock lock(guard_);

    FlatSet<uint8_t>::iterator begin = populated_.begin();
    FlatSet<uint8_t>::iterator it = begin;

    while (it != populated_.end()) {
      uint8_t index = *it;
      EditInfo& edit_info = edits_[index];
      if (edit_info.time <= clean_time && edit_info.time != UINT64_C(0)) {
        // Just skip it if we find something being edited at a time
        // overlapping our access_time.
        it++;
        continue;
      }

      MapVersion& current_info = current_values_[index];
      MapVersion& historical_info = historical_values_[index];
      bool current_map_cleared = false;
      if (current_info.times.remove < clean_time) {
        delete current_info.value;
        current_info.value = nullptr;
        current_info.times.create = UINT64_C(0);
        current_info.times.remove = UINT64_C(0);
        current_map_cleared = true;
      }
      else if (current_info.times.create <= clean_time) {
        current_info.value->CleanVersions(clean_time);
      }

      if (historical_info.times.remove <= clean_time) {
        delete historical_info.value;
        historical_info.value = nullptr;
        historical_info.times.create = UINT64_C(0);
        historical_info.times.remove = current_info.times.create;
        if (current_map_cleared) {
          populated_.erase(index);
        } else {
          it++;
        }
      } else if (historical_info.times.create <= clean_time) {
        current_info.value->CleanVersions(clean_time);
        it++;
      }
    }
  }

  void CompleteWriteOperation(const Transaction* tx) {
    acumio::transaction::ExclusiveLock lock(guard_);
    CompleteWriteOperationWithGuard(tx);
  }

  void Rollback(const Transaction* tx) {
    acumio::transaction::ExclusiveLock lock(guard_);
    RollbackWithGuard(tx);
  }

 private:

  /***************** Private typedefs and structs ******************/
  enum EditState {
    NOT_EDITING = 0,
    // Any number of transactions can be involved in a PASS_THROUGH
    // state. If after multiple transactions a new transaction would
    // cause a BURSTING state, the BURSTING is rejected.
    PASS_THROUGH = 1,
    // Only one transaction can be involved in a CREATING edit state.
    CREATING = 2,
    // Only one transaction can be involved ina BURSTING edit state.
    BURSTING = 3
  };

  struct EditInfo {
    EditInfo() : state(NOT_EDITING), time(UINT64_C(0)), edit_value(nullptr),
        edit_transactions() {}
    EditInfo(const EditInfo& o) : state(o.state), time(o.time),
        edit_value(o.edit_value), edit_transactions(o.edit_transactions) {}
    ~EditInfo() { delete edit_value; }

    EditState state;
    uint64_t time;
    TxManagedMap<EltType>* edit_value;
    FlatSet<const Transaction*, TxPointerLess> edit_transactions;
  };

  struct MapVersion {
    MapVersion(TxManagedMap<EltType>* v,
               const typename TxAware<EltType>::TimeBoundary& t) :
      value(v), times(t) {}
    MapVersion() : value(nullptr), times(UINT64_C(0), UINT64_C(0)) {}
    MapVersion(const MapVersion& other) : value(other.value),
        times(other.times) {}
    ~MapVersion() { delete value; }

    TxManagedMap<EltType>* value;
    typename TxAware<EltType>::TimeBoundary times;
  };

  typedef std::multimap<Transaction::Id, uint8_t>::iterator TxEditsIterator;

  /************************* Private methods ****************************/
  std::pair<TxEditsIterator, TxEditsIterator>
  TxEditsRange(const Transaction* tx) {
    std::pair<TxEditsIterator, TxEditsIterator> ret_val;
    ret_val.first = tx_edits_map_.end();
    ret_val.second = tx_edits_map_.end();

    if (tx == nullptr) {
      // TODO: Log this condition. In the context where this happens, it
      // usually represents an error condition.
      return ret_val;
    }

    TxEditsIterator it = tx_edits_map_.lower_bound(tx->id());
    if (it == tx_edits_map_.end() || it->first != tx->id()) {
      // This is a normal condition. It indicates that the transaction
      // has already been pushed to completion or rolled back for this node.
      return ret_val;
    }

    ret_val.first = it;
    ret_val.second = tx_edits_map_.upper_bound(tx->id());
    return ret_val;
  }

  inline uint8_t IndexFromKey(const char* key) const {
    // fwiw, static_cast and implicit cast are the same. The idea here is
    // just to signal intent: we really do mean to treat the char as an
    // unsigned 8-bit number.
    return static_cast<uint8_t>(key[0]);
  }

  void CompleteWriteOperationWithGuard(const Transaction* tx) {
    std::pair<TxEditsIterator, TxEditsIterator> range = TxEditsRange(tx);

    for (TxEditsIterator it = range.first; it != range.second; ++it) {
      uint8_t index = it->second;
      EditInfo& edit_info = edits_[index];
      edit_info.edit_transactions.erase(tx);
      switch (edit_info.state) {
        case NOT_EDITING:
          break;
        case PASS_THROUGH:
          (current_values_[index]).value->CompleteWriteOperation(tx);
          break;
        case CREATING:
          // We are relying on state transition information here that says
          // that whenever we do this assignment, the history information
          // will be nullptr, and prior to that, is also a nullptr, so
          // we don't need to update it.
          historical_values_[index].times.remove = edit_info.time;
          current_values_[index].times.create = edit_info.time;
          current_values_[index].times.remove = acumio::time::END_OF_TIME;
          current_values_[index].value = edit_info.edit_value;
          edit_info.edit_value = nullptr;
          edit_info.time = UINT64_C(0);
          edit_info.state = NOT_EDITING;
          // edit_transactions should already be empty.
          populated_.insert(index);
          break;
        case BURSTING:
          historical_values_[index].value = current_values_[index].value;
          historical_values_[index].times.create =
              current_values_[index].times.create;
          historical_values_[index].times.remove = edit_info.time;
          current_values_[index].value = edit_info.edit_value;
          current_values_[index].times.create = edit_info.time;
          current_values_[index].times.remove = acumio::time::END_OF_TIME;
          edit_info.edit_value = nullptr;
          edit_info.state = NOT_EDITING;
          edit_info.time = UINT64_C(0);
          // edit_transactions should already be empty.
          // populated_ should already contain the index.
          break;
        default:
          // TODO: Log a fatal error here. This should never happen, and
          //       indicates that we failed to update this case-statement
          //       to handle a new enum value. Either that or edit_info.state
          //       has been corrupted to an illegal value.
          return;
      }
    }
  }

  void RollbackWithGuard(const Transaction* tx) {
    std::pair<TxEditsIterator, TxEditsIterator> range = TxEditsRange(tx);

    for (TxEditsIterator it = range.first; it != range.second; ++it) {
      uint8_t index = it->second;
      EditInfo& edit_info = edits_[index];
      // This will be a no-op if the tx is not present.
      edit_info.edit_transactions.erase(tx);
      if (edit_info.state != NOT_EDITING) {
        if (edit_info.state == PASS_THROUGH) {
          (current_values_[index]).value->Rollback(tx);
        }

        if (edit_info.edit_transactions.size() == 0) {
          edit_info.state = NOT_EDITING;
          edit_info.time = UINT64_C(0);
          delete edit_info.edit_value;
          edit_info.edit_value = nullptr;
        }
      }
    }
  }

  grpc::Status VerifyNoConflictingEdits(const Transaction* tx,
      EditInfo& edit_info, MapVersion& current_version, uint64_t tx_time) {
    FlatSet<const Transaction*, TxPointerLess>& edit_transactions =
        edit_info.edit_transactions;
    // Counter-intuitively, we do not increment the iterator in this loop.
    // The reason is that the rollback and completion operations each
    // decrease the size of the list. Instead, we will increment inside the
    // loop for those cases that need it.
    auto it = edit_transactions.begin();
    while (it != edit_transactions.end()) {
      const Transaction* current_tx = *it;
      Transaction::AtomicInfo current_state_time = current_tx->GetAtomicInfo();
      Transaction::State tx_state = current_state_time.state;
      switch(tx_state) {
        case Transaction::NOT_STARTED: // Intentional fall-through to next case.
        case Transaction::READ:
          // If we reach this condition, we have a re-purposed transaction
          // that was not properly rolled back previously. Push the rollback
          // operation forward.
          RollbackWithGuard(current_tx);
          break;
        case Transaction::WRITE:
          if (edit_info.state == CREATING || edit_info.state == BURSTING) {
            if (current_tx->id() != tx->id()) {
              if (current_state_time.operation_start_time  == edit_info.time) {
                return grpc::Status(grpc::StatusCode::ABORTED,
                                    "concurrency exception.");
              } else {
                RollbackWithGuard(current_tx);
              }
            }
          } else if (edit_info.state == NOT_EDITING) {
            RollbackWithGuard(current_tx);
          } else {
            // assert: edit_info.state == PASS_THROUGH. Skip past this
            // transaction and continue looking at others.
            it++;
          }
          break;
        case Transaction::COMPLETING_WRITE:
          CompleteWriteOperationWithGuard(current_tx);
          break;
        case Transaction::COMMITTED:
          // If we reach here, we have probably encountered the cruft of a
          // previously timed-out transaction where the transaction has since
          // then been re-purposed and then committed.
          RollbackWithGuard(current_tx);
          break;
        case Transaction::ROLLED_BACK:
          RollbackWithGuard(current_tx);
          break;
        default:
          return grpc::Status(grpc::StatusCode::INTERNAL,
                              "Coding issue. Did not fix switch.");
      }
    }

    if (tx_time < current_version.times.create ||
        (current_version.times.remove != acumio::time::END_OF_TIME &&
         tx_time < current_version.times.remove)) {
      return grpc::Status(grpc::StatusCode::ABORTED, "concurrency exception.");
    }

    return grpc::Status::OK;
  }

  grpc::Status BurstAndAdd(const char* key, uint32_t value, EditInfo& edit_info,
      MapVersion& current_version, const Transaction* tx, uint64_t tx_time) {
    edit_info.edit_value =
        intermediate_factory_->CreateCopy(current_version.value,
                                          tx_time);
    grpc::Status ret_val = edit_info.edit_value->Add(key, value, tx, tx_time);
    if (!ret_val.ok()) {
      delete edit_info.edit_value;
      edit_info.edit_value = nullptr;
      return ret_val;
    }

    edit_info.edit_transactions.insert(tx);
    edit_info.state = BURSTING;
    edit_info.time = tx_time;
    return grpc::Status::OK;
  }

  grpc::Status BurstAndRemove(const char* key, EditInfo& edit_info,
      MapVersion& current_version, const Transaction* tx, uint64_t tx_time) {
    edit_info.edit_value =
        intermediate_factory_->CreateCopy(current_version.value,
                                          tx_time);
    grpc::Status ret_val = edit_info.edit_value->Remove(key, tx, tx_time);
    if (!ret_val.ok()) {
      delete edit_info.edit_value;
      edit_info.edit_value = nullptr;
      return ret_val;
    }

    edit_info.edit_transactions.insert(tx);
    edit_info.state = BURSTING;
    edit_info.time = tx_time;
    return grpc::Status::OK;
  }

  grpc::Status BurstAndRemove(const char* key, uint32_t value,
      EditInfo& edit_info, MapVersion& current_version, const Transaction* tx,
      uint64_t tx_time) {
    edit_info.edit_value =
        intermediate_factory_->CreateCopy(current_version.value,
                                          tx_time);
    grpc::Status ret_val = edit_info.edit_value->Remove(key, value, tx,
                                                        tx_time);
    if (!ret_val.ok()) {
      delete edit_info.edit_value;
      edit_info.edit_value = nullptr;
      return ret_val;
    }

    edit_info.edit_transactions.insert(tx);
    edit_info.state = BURSTING;
    edit_info.time = tx_time;
    return grpc::Status::OK;
  }

  grpc::Status BurstAndReplace(const char* key, uint32_t value,
      EditInfo& edit_info, MapVersion& current_version, const Transaction* tx,
      uint64_t tx_time) {
    edit_info.edit_value =
        intermediate_factory_->CreateCopy(current_version.value,
                                          tx_time);
    grpc::Status ret_val = edit_info.edit_value->Replace(key, value, tx,
                                                         tx_time);
    if (!ret_val.ok()) {
      delete edit_info.edit_value;
      edit_info.edit_value = nullptr;
      return ret_val;
    }

    edit_info.edit_transactions.insert(tx);
    edit_info.state = BURSTING;
    edit_info.time = tx_time;
    return grpc::Status::OK;
  }

  // Assumes you already have a read-lock.
  const TxManagedMap<EltType>* MapVersionAtTime(uint8_t index,
                                                uint64_t access_time) const {
    const EditInfo& edit_info = edits_[index];

    if ((edit_info.state == CREATING || edit_info.state == BURSTING) &&
        (edit_info.time <= access_time && edit_info.time != UINT64_C(0))) {
      const Transaction* tx = *(edit_info.edit_transactions.begin());
      Transaction::AtomicInfo tx_info;
      uint64_t tx_complete_time;
      tx->GetAtomicInfo(&tx_info, &tx_complete_time);

      if (tx_info.operation_start_time == edit_info.time &&
          tx_info.state == Transaction::COMPLETING_WRITE &&
          tx_complete_time <= access_time) {
        return edit_info.edit_value;
      }
    }

    const MapVersion& current_info = current_values_[index];
    if (current_info.times.create <= access_time) {
      if (current_info.times.remove <= access_time) {
        return nullptr;
      }

      return current_info.value;
    }

    const MapVersion& historical_info = historical_values_[index];
    if (historical_info.times.create > access_time ||
        historical_info.times.remove <= access_time) {
      return nullptr;
    }

    return historical_info.value;
  }

  // Assumes you already have a read-lock.
  std::unique_ptr<TxBasicIterator>
  IndexedBeginIterator(uint8_t index, uint64_t access_time) const {
    const TxManagedMap<EltType>* map = MapVersionAtTime(index, access_time);
    if (map == nullptr) {
      return std::unique_ptr<TxBasicIterator>(nullptr);
    }

    return map->Begin(access_time);
  }

  // Assumes you already have a read-lock.
  std::unique_ptr<TxBasicIterator>
  IndexedReverseBeginIterator(uint8_t index, uint64_t access_time) const {
    const TxManagedMap<EltType>* map = MapVersionAtTime(index, access_time);
    if (map == nullptr) {
      return map->Begin(access_time);
    }

    return map->ReverseBegin(access_time);
  }

  // Assumes you already have a read-lock.
  std::unique_ptr<TxBasicIterator>
  IndexedEndIterator(uint8_t index, uint64_t access_time) const {
    const TxManagedMap<EltType>* map = MapVersionAtTime(index, access_time);
    if (map == nullptr) {
      return std::unique_ptr<TxBasicIterator>(nullptr);
    }

    return map->End(access_time);
  }

  // Assumes you already have a read-lock.
  std::unique_ptr<TxBasicIterator>
  IndexedLowerBoundIterator(const char* key, uint64_t access_time) const {
    uint8_t index = IndexFromKey(key);
    const TxManagedMap<EltType>* map = MapVersionAtTime(index, access_time);
    if (map == nullptr) {
      return std::unique_ptr<TxBasicIterator>(nullptr);
    }

    return map->LowerBound(key + 1, access_time);
  }

  // ****************  Primary Private Attributes *********************** //
  mutable SharedMutex guard_;

  TxManagedMapFactory<EltType>* leaf_factory_;
  TxManagedMapFactory<EltType>* intermediate_factory_;

  // For each index, there will only be a single historical_value at most.
  // Possible configurations:
  //   1. current value is nullptr with date-range [0,end-of-time)
  //      history value is nullptr with date-range [0,0)
  //   2. current value is leaf-level with date-range [t1,end-of-time)
  //      history value is nullptr with date-range [0,t1)
  //   3. current value is intermediate with date-range [t2,end-of-time)
  //      history value is leaf-level with date-range [t1,t2)
  MapVersion current_values_[256];
  MapVersion historical_values_[256];

  // This reflects the union of current_values_ and historical_values_.
  // The point of this set is to speed up the process of iterating over
  // all values. If x is an element of populated_ then either
  // current_values_[x] is populated with a map or historical_values[x]
  // is populated with a map.
  FlatSet<uint8_t> populated_;

  // **************** Edit Private Attributes ************************* //
  // When CompleteWriteOperation or Rollback gets invoked, we need to be
  // able to efficiently locate all edits related to the operation. The
  // tx_edits_map_ will enable that. When reading data, we need to see which
  // version to read. The edits_ variable will provide that.
  std::multimap<Transaction::Id, uint8_t> tx_edits_map_;
  EditInfo edits_[256];
};

} // namespace collection
} // acumio

#endif // AcumioServer_tx_aware_trie_node_h
