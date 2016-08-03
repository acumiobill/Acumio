#ifndef AcumioServer_tx_aware_burst_trie_h
#define AcumioServer_tx_aware_burst_trie_h
//============================================================================
// Name        : tx_aware_burst_trie.h
// Author      : Bill Province (bill@acumio.com)
// Version     :
// Copyright   : Copyright (C) 2016 Acumio
// Description : This file contains the definition for a
//               TxAwareBurstTrie data structure.
//               Based in part on "HAT-trie: A Cache-conscious Trie-based
//               Data Structure for Strings" (Nikolas Askitis, Ranjan Sinha).
//               In addition, this structure contains "transaction-awareness,"
//               which in our case means that it supports a MVCC model.
//               This will potentially allow multiple readers and one
//               active writer per index node. While write operations occur,
//               we want to make sure that we still have the ability to
//               perform read operations.
//
//============================================================================

#include <grpc++/support/status.h>
#include <memory>
#include <set>
#include <string>
#include "object_allocator.h"
#include "shared_mutex.h"
#include "tx_aware_flat_map.h"
#include "tx_aware_trie_node.h"
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
class TxAwareBurstTrie : public TxManagedMap<EltType> {
 public:
  TxAwareBurstTrie() {}
  ~TxAwareBurstTrie() {}

  grpc::Status GetValuePosition(const char* key, uint32_t* value,
                                uint64_t access_time) const {
    // TODO: STUB!
    return grpc::Status::OK;
  }

  uint32_t Size(uint64_t access_time, bool* exists_at_time) const {
    // TODO: STUB!
    return 0;
  }

  grpc::Status Add(const char* key, uint32_t value, const Transaction* tx,
                   uint64_t tx_time) {
    // TODO: STUB!
    return grpc::Status::OK;
  }

  grpc::Status Remove(const char* key, const Transaction* tx,
                      uint64_t tx_time) {
    // TODO: STUB!
    return grpc::Status::OK;
  }

  grpc::Status Remove(const char* key, uint32_t value, const Transaction* tx,
                      uint64_t tx_time) {
    // TODO: STUB!
    return grpc::Status::OK;
  }

  grpc::Status Replace(const char* key, uint32_t value, const Transaction* tx,
                       uint64_t tx_time) {
    // TODO: STUB!
    return grpc::Status::OK;
  }

  std::unique_ptr<TxBasicIterator> Begin(uint64_t access_time) const {
    // TODO: STUB!
    return std::unique_ptr<TxBasicIterator>(nullptr);
  }

  std::unique_ptr<TxBasicIterator> ReverseBegin(uint64_t access_time) const {
    // TODO: STUB!
    return std::unique_ptr<TxBasicIterator>(nullptr);
  }

  std::unique_ptr<TxBasicIterator> End(uint64_t access_time) const {
    // TODO: STUB!
    return std::unique_ptr<TxBasicIterator>(nullptr);
  }

  std::unique_ptr<TxBasicIterator> LowerBound(const char* key,
                                              uint64_t access_time) const {
    // TODO: STUB!
    return std::unique_ptr<TxBasicIterator>(nullptr);
  }

  virtual void CleanVersions(uint64_t clean_time) {
    // TODO: STUB!
  }

  virtual void CompleteWriteOperation(const Transaction* tx) {
    // TODO: STUB!
  }

  virtual void Rollback(const Transaction* tx) {
    // TODO: STUB!
  }

 private:
  uint64_t cleanup_nanos_;
  uint64_t max_leaf_key_space_;
  uint8_t max_leaf_size_;
  mutable SharedMutex guard_;
  TxAwareTrieNode<EltType> root_;
};

} // namespace collection
} // acumio

#endif // AcumioServer_tx_aware_burst_trie_h
