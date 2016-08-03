//============================================================================
// Name        : tx_managed_map.cpp
// Author      : Bill Province (bill@acumio.com)
// Version     :
// Copyright   : Copyright (C) 2016 Acumio
// Description : TxManagedMap implementation.
//============================================================================
#include "tx_managed_map.h"

namespace acumio {
namespace collection {

UnadaptedTxManagedMap::UnadaptedTxManagedMap(bool allow_duplicates) :
    allow_duplicates_(allow_duplicates) {}

UnadaptedTxManagedMap::UnadaptedTxManagedMap() :
    UnadaptedTxManagedMap(false) {}

UnadaptedTxManagedMap::~UnadaptedTxManagedMap() {}

void UnadaptedTxManagedMap::RegisterAddOpWithTransaction(
    const char* key, uint32_t value,
    WriteTransaction* tx, uint64_t tx_time) {
  tx->AddOperation(AddCallback(key, value, tx_time),
                   CompleteWriteCallback(),
                   RollbackCallback());
}

void UnadaptedTxManagedMap::RegisterReplaceOpWithTransaction(const char* key,
    uint32_t value, WriteTransaction* tx, uint64_t tx_time) {
  tx->AddOperation(ReplaceCallback(key, value, tx_time),
                   CompleteWriteCallback(),
                   RollbackCallback());
}

void UnadaptedTxManagedMap::RegisterRemoveOpWithTransaction(const char* key,
    WriteTransaction* tx, uint64_t tx_time) {
  tx->AddOperation(RemoveCallback(key, tx_time),
                   CompleteWriteCallback(),
                   RollbackCallback());
}

void UnadaptedTxManagedMap::RegisterRemoveOpWithTransaction(const char* key,
    uint32_t value, WriteTransaction* tx, uint64_t tx_time) {
  tx->AddOperation(RemoveCallback(key, value, tx_time),
                   CompleteWriteCallback(),
                   RollbackCallback());
}

WriteTransaction::OpFunction UnadaptedTxManagedMap::AddCallback(
    const char* key, uint32_t value, uint64_t tx_time) {
  grpc::Status (UnadaptedTxManagedMap::*fn)(const char*, uint32_t,
      const Transaction*, uint64_t)  = &UnadaptedTxManagedMap::Add;
  return std::bind(fn, this, key, value, std::placeholders::_1, tx_time);
}

WriteTransaction::OpFunction UnadaptedTxManagedMap::ReplaceCallback(
    const char* key, uint32_t value, uint64_t tx_time) {
  grpc::Status (UnadaptedTxManagedMap::*fn)(const char*, uint32_t,
      const Transaction*, uint64_t)  = &UnadaptedTxManagedMap::Replace;
  return std::bind(fn, this, key, value, std::placeholders::_1, tx_time);
}

WriteTransaction::OpFunction UnadaptedTxManagedMap::RemoveCallback(
    const char* key, uint64_t tx_time) {
  grpc::Status (UnadaptedTxManagedMap::*fn)(const char*,
      const Transaction*, uint64_t) = &UnadaptedTxManagedMap::Remove;
  return std::bind(fn, this, key, std::placeholders::_1, tx_time);
}

WriteTransaction::OpFunction UnadaptedTxManagedMap::RemoveCallback(
    const char* key, uint32_t value, uint64_t tx_time) {
  grpc::Status (UnadaptedTxManagedMap::*fn)(const char*,
      uint32_t, const Transaction*, uint64_t)  = &UnadaptedTxManagedMap::Remove;
  return std::bind(fn, this, key, value, std::placeholders::_1, tx_time);
}

WriteTransaction::CompletionFunction
UnadaptedTxManagedMap::CompleteWriteCallback() {
  void (UnadaptedTxManagedMap::*fn)(const Transaction*) =
      &UnadaptedTxManagedMap::CompleteWriteOperation;
  return std::bind(fn, this, std::placeholders::_1);
}

WriteTransaction::CompletionFunction UnadaptedTxManagedMap::RollbackCallback() {
  void (UnadaptedTxManagedMap::*fn)(const Transaction*) =
      &UnadaptedTxManagedMap::Rollback;
  return std::bind(fn, this, std::placeholders::_1);
}

} // namespace collection
} // namespace acumio
