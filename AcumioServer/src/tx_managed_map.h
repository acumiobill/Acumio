#ifndef AcumioServer_tx_managed_map_h
#define AcumioServer_tx_managed_map_h
//============================================================================
// Name        : tx_managed_map.h
// Author      : Bill Province (bill@acumio.com)
// Version     :
// Copyright   : Copyright (C) 2016 Acumio
// Description : Provides some simple utilities for working with entities
//               managed via the ObjectAllocator mechanism.
//============================================================================

#include <grpc++/support/status.h>
#include <memory>
#include <string>

#include "iterators.h"
#include "object_allocator.h"
#include "rope_piece.h"
#include "shared_mutex.h"
#include "transaction.h"


namespace acumio {
namespace collection {
using acumio::string::RopePiece;
using acumio::transaction::SharedLock;
using acumio::transaction::SharedMutex;
using acumio::transaction::Transaction;
using acumio::transaction::WriteTransaction;

struct MapIterElement {
  MapIterElement(const char* k, uint32_t v) : key(k), value(v) {}
  MapIterElement(const RopePiece& k, uint32_t v) : key(k), value(v) {}
  MapIterElement() : MapIterElement(nullptr, UINT32_C(0)) {}

  RopePiece key;
  uint32_t value;
};

typedef BasicIterator<MapIterElement> TxBasicIterator;
typedef DelegatingIterator<MapIterElement> TxManagedIterator;

class UnadaptedTxManagedMap {
 public:
  UnadaptedTxManagedMap(bool allow_duplicates);

  // Same as UnadaptedTxManagedMap(false);
  UnadaptedTxManagedMap();

  virtual ~UnadaptedTxManagedMap();

  virtual grpc::Status GetValuePosition(const char* key, uint32_t* value,
                           uint64_t access_time) const = 0;
  inline grpc::Status GetValuePosition(std::string& key, uint32_t* value,
                           uint64_t access_time) {
    return GetValuePosition(key.c_str(), value, access_time);
  }

  inline bool allow_duplicates() const { return allow_duplicates_; }

  virtual uint32_t Size(uint64_t access_time, bool* exists_at_time) const = 0;

  inline bool Empty(uint64_t access_time) {
    bool exists_at_time = false;
    return (Size(access_time, &exists_at_time) == 0);
  }

  virtual grpc::Status Add(const char* key, uint32_t value,
                           const Transaction* tx, uint64_t tx_time) = 0;

  inline grpc::Status Add(const std::string& key, uint32_t value,
                          const Transaction* tx, uint64_t tx_time) {
    return Add(key.c_str(), value, tx, tx_time);
  }

  virtual grpc::Status Remove(const char* key, const Transaction* tx,
                              uint64_t tx_time) = 0;

  inline grpc::Status Remove(const std::string& key, const Transaction* tx,
                             uint64_t tx_time) {
    return Remove(key.c_str(), tx, tx_time);
  }

  virtual grpc::Status Remove(const char* key, uint32_t value,
                              const Transaction* tx, uint64_t tx_time) = 0;

  inline grpc::Status Remove(const std::string& key, uint32_t value,
                             const Transaction* tx, uint64_t tx_time) {
    return Remove(key.c_str(), value, tx, tx_time);
  }

  virtual grpc::Status Replace(const char* key, uint32_t value,
                               const Transaction* tx, uint64_t tx_time) = 0;

  inline grpc::Status Replace(const std::string& key, uint32_t value,
                              const Transaction* tx, uint64_t tx_time) {
    return Replace(key.c_str(), value, tx, tx_time);
  }

  virtual
  std::unique_ptr<TxBasicIterator> Begin(uint64_t access_time) const = 0;

  virtual
  std::unique_ptr<TxBasicIterator> ReverseBegin(uint64_t access_time) const = 0;

  virtual std::unique_ptr<TxBasicIterator> End(uint64_t access_time) const = 0;

  virtual
  std::unique_ptr<TxBasicIterator> LowerBound(const char* key,
                                              uint64_t access_time) const = 0;

  inline std::unique_ptr<TxBasicIterator> LowerBound(
      const std::string& key, uint64_t access_time) const {
    return LowerBound(key.c_str(), access_time);
  }

  virtual void CleanVersions(uint64_t clean_time) = 0;

  virtual void CompleteWriteOperation(const Transaction* tx) = 0;
  virtual void Rollback(const Transaction* tx) = 0;

  // This takes care of the operation of registering the triple:
  // AddCallback, CompleteWriteCallback, and RollbackCallback with
  // the Transaction.
  void RegisterAddOpWithTransaction(const char* key, uint32_t value,
                                    WriteTransaction* tx, uint64_t tx_time);

  void RegisterReplaceOpWithTransaction(
      const char* key, uint32_t value, WriteTransaction* tx, uint64_t tx_time);

  void RegisterRemoveOpWithTransaction(
      const char* key, WriteTransaction* tx, uint64_t tx_time);

  void RegisterRemoveOpWithTransaction(
      const char* key, uint32_t value, WriteTransaction* tx, uint64_t tx_time);

 private:
  WriteTransaction::OpFunction AddCallback(const char* key, uint32_t value,
                                           uint64_t tx_time);

  WriteTransaction::OpFunction ReplaceCallback(
      const char* key, uint32_t value, uint64_t tx_time);

  WriteTransaction::OpFunction RemoveCallback(const char* key,
                                              uint64_t tx_time);

  WriteTransaction::OpFunction RemoveCallback(const char* key, uint32_t value,
                                              uint64_t tx_time);

  WriteTransaction::CompletionFunction CompleteWriteCallback();

  WriteTransaction::RollbackFunction RollbackCallback();

  // Private Member Attributes. Since this is pure virtual, it is of course,
  // pretty sparse.
  bool allow_duplicates_;  
};

template <typename EltType>
class TxManagedMap : public UnadaptedTxManagedMap {
 public:
  // object_allocator is expected to be shared between TxManagedMap objects,
  // and is owned externally to the TxManagedMap.
  TxManagedMap(ObjectAllocator<EltType>* object_allocator,
               bool allow_duplicates = false) :
      UnadaptedTxManagedMap(allow_duplicates),
      object_allocator_(object_allocator) {}
  TxManagedMap() : TxManagedMap(nullptr, false) {}

  inline const EltType& GetValue(uint32_t allocated_position) const {
    return object_allocator_->ObjectAt(allocated_position);
  }

  // Typical usage:
  //   EltType value;
  //   grpc::Status result = Get(key, &value, access_time);
  grpc::Status Get(const char* key, EltType* value,
                   uint64_t access_time) const {
    uint32_t value_position;
    grpc::Status ret_val = GetValuePosition(key, &value_position, access_time);
    if (ret_val.ok()) {
      *value = object_allocator_->ObjectAt(value_position);
    }
    return ret_val;
  }

  // Typical usage:
  //   EltType value;
  //   grpc::Status result = Get(key, &value, access_time);
  inline grpc::Status Get(const std::string& key, EltType* value,
                          uint64_t access_time) const {
    return Get(key.c_str(), value, access_time);
  }

  // Typical usage:
  //   EltType* value;
  //   grpc::Status result = Get(key, &value, access_time);
  //   if (result.ok()) {
  //     value->modify_something();
  //   }
  grpc::Status GetModifiableValue(const char* key, EltType** value,
                                  uint64_t access_time) const {
    uint32_t value_position;
    grpc::Status ret_val = GetValuePosition(key, &value_position, access_time);
    if (ret_val.ok()) {
      *value = &(object_allocator_->ObjectAt(value_position));
    }
    return ret_val;
  }

 protected:
  // Derived classes need access to this so that they can add and remove
  // references to allocated objects.
  ObjectAllocator<EltType>* object_allocator_;
};

} // namespace collection
} // namespace acumio

#endif // AcumioServer_tx_managed_map_h
