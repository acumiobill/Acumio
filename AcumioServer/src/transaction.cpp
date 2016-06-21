//============================================================================
// Name        : transaction.cpp
// Author      : Bill Province (bill@acumio.com)
// Version     :
// Copyright   : Copyright (C) 2016 Acumio
// Description : Transaction implementation.
//============================================================================
#include "transaction.h"

#include <atomic>
#include <cstdint> // needed for UINT64_C(x) macro.

// iostream is ~sed for std::cout. Comment-out when not debugging.
// Usually, commented-out code is a bad sign, but mostly okay if we use it
// frequently on a temporary basis.
// #include <iostream>
#include <mutex>
#include <sstream>

#include "time_util.h"

namespace acumio {
namespace transaction {

Transaction::Transaction(const acumio::test::TestHook<Transaction*>* hook,
                         Id id) :
    hook_(hook), id_(id), operation_complete_time_(UINT64_C(0)) {
  Transaction::AtomicInfo new_info;
  new_info.operation_start_time = UINT64_C(0);
  new_info.state = Transaction::NOT_STARTED;
  info_.store(new_info);
}
Transaction::~Transaction() {}

bool Transaction::BeginRead(uint64_t read_start_time) {
  Transaction::AtomicInfo expected_info;
  expected_info.operation_start_time = UINT64_C(0);
  expected_info.state = Transaction::NOT_STARTED;
  Transaction::AtomicInfo result_info;
  result_info.operation_start_time = read_start_time;
  result_info.state = Transaction::READ;
  hook_->invoke_1(this); // See test_hooks.h
  return info_.compare_exchange_strong(expected_info, result_info,
                                       std::memory_order_relaxed);
}

bool Transaction::BeginWrite(uint64_t write_start_time) {
  Transaction::AtomicInfo expected_info;
  expected_info.operation_start_time = UINT64_C(0);
  expected_info.state = Transaction::NOT_STARTED;
  Transaction::AtomicInfo result_info;
  result_info.operation_start_time = write_start_time;
  result_info.state = Transaction::WRITE;
  hook_->invoke_2(this); // See test_hooks.h
  return info_.compare_exchange_strong(expected_info, result_info,
                                       std::memory_order_relaxed);
}

bool Transaction::StartWriteComplete(uint64_t expected_write_time) {
  return StartWriteComplete(expected_write_time,
                            acumio::time::TimerNanosSinceEpoch());
}

bool Transaction::StartWriteComplete(uint64_t expected_write_time,
                                     uint64_t effective_commit_time) {
  Transaction::AtomicInfo expected_info;
  expected_info.state = Transaction::WRITE;
  expected_info.operation_start_time = expected_write_time;
  Transaction::AtomicInfo result_info;
  result_info.state = Transaction::COMPLETING_WRITE;
  result_info.operation_start_time = expected_write_time;
  hook_->invoke_3(this); // See test_hooks.h
  if (info_.compare_exchange_strong(expected_info, result_info,
                                    std::memory_order_relaxed)) {
    // This is a known data-race condition, and we are not going to attempt
    // to repair it. If the thread suspends right at this point: after
    // swapping the atomic content, but before setting
    // operation_complete_time_ - we could have a data race. In particular,
    // if the suspension time is long enough that the transaction gets
    // reaped. However, the reap-time vs completion of write time data
    // race is built into the design, so nothing changes that.
    operation_complete_time_ = effective_commit_time;
    return true;
  }
  return false;
}

bool Transaction::Commit(uint64_t expected_start_time) {
  Transaction::AtomicInfo expected_info = info_.load(std::memory_order_relaxed);
  if (expected_info.operation_start_time != expected_start_time ||
      (expected_info.state != Transaction::COMPLETING_WRITE &&
       expected_info.state != Transaction::READ)) {
    return false;
  }
  Transaction::AtomicInfo update_info;
  update_info.state = Transaction::COMMITTED;
  update_info.operation_start_time = expected_start_time;
  
  switch (expected_info.state) {
    case Transaction::READ: break;
    case Transaction::COMPLETING_WRITE: break;
    default:
      return false;
  }

  return info_.compare_exchange_strong(expected_info, update_info,
                                       std::memory_order_relaxed);
}

bool Transaction::Rollback(uint64_t expected_start_time) {
  Transaction::AtomicInfo expected_info = info_.load(std::memory_order_relaxed);
  if (expected_info.operation_start_time != expected_start_time ||
      (expected_info.state != WRITE && expected_info.state != READ)) {
    return false;
  }
  Transaction::AtomicInfo update_info;
  update_info.state = Transaction::ROLLED_BACK;
  update_info.operation_start_time = expected_start_time;
  
  switch (expected_info.state) {
    case READ: break;
    case WRITE: break;
    default:
      return false;
  }

  return info_.compare_exchange_strong(expected_info, update_info,
                                       std::memory_order_relaxed);
}

bool Transaction::Reset(uint64_t expected_start_time) {
  Transaction::AtomicInfo expected_info = info_.load(std::memory_order_relaxed);
  if (expected_info.operation_start_time != expected_start_time) {
    return false;
  }
  Transaction::AtomicInfo update_info;
  update_info.state = Transaction::NOT_STARTED;
  update_info.operation_start_time = UINT64_C(0);
  if (info_.compare_exchange_strong(expected_info, update_info,
                                    std::memory_order_relaxed)) {
    operation_complete_time_ = UINT64_C(0);
    return true;
  }
  return false;
}

bool Transaction::ResetIfOld(uint64_t youngest_reset) {
  Transaction::AtomicInfo current_info = info_.load(std::memory_order_relaxed);
  uint64_t max_op_time = (current_info.operation_start_time >
                          operation_complete_time_ ?
                          current_info.operation_start_time :
                          operation_complete_time_);
  hook_->invoke_4(this); // See test_hooks.h
  // Unfortunately, there is no way to atomically update the current info
  // based on this comparison. For that reason, we simply first establish
  // the age difference, and if this transaction is old enough, we proceed
  // with the reset. If by chance, the presumed "dead" thread wakes up and
  // finishes its work, there will be no harm done in performing this check.
  if (max_op_time > youngest_reset) {
    return false;
  }

  hook_->invoke_5(this); // See test_hooks.h
  return Reset(current_info.operation_start_time);
}

ReadTransaction::ReadTransaction(TransactionManager& manager) :
  manager_(manager), tx_(nullptr), read_start_time_(UINT64_C(0)),
  done_(false) {
  tx_ = manager.StartReadTransaction(&read_start_time_);
}

grpc::Status ReadTransaction::Commit() {
  if (! done_) {
    done_ = true;
    if (tx_ == nullptr) {
      return grpc::Status(grpc::StatusCode::ABORTED,
          "It apparently took too long to initialize the read transaction. "
          "This is usually a sign that the reap_timeout value is set too "
          "short, but may also be indicative of an overburdened system. "
          "Consider increasing resources for the server, and/or modifying "
          "the reap_timeout parameter.");
    }

    if(tx_->Commit(read_start_time_)) {
      manager_.hook()->invoke_6(tx_); // See test_hooks.h
      // If tx_ gets repurposed between the time of commit and this Release
      // call, we still consider this operation successful. However, it
      // suggests that the system might be slightly overloaded, so we should
      // set a monitor variable for that condition.
      // TODO: Increment monitor variable if Release returns false, and log
      //       a warning in that case.
      manager_.Release(tx_, read_start_time_);
      return grpc::Status::OK;
    }
    // This check is a bit imprecise, but sufficient for the most part.
    // The problem with this check is that time will have passed since
    // the invocation of tx_->Commit and this check, so while we may find
    // now that we have timed out, we may be hiding a more serious error.
    // This is a relatively minor issue as in either case, we return a
    // failed status, and if a more serious issue did indeed occur, it will
    // likely fail again.
    if (read_start_time_ <= acumio::time::LatestTimeoutTime(
            manager_.get_timeout_nanos())) {
      return grpc::Status(grpc::StatusCode::DEADLINE_EXCEEDED,
                          "The transaction appears to have been timed out.");
    }
    return grpc::Status(grpc::StatusCode::INTERNAL,
                        "The transaction state has been changed by another "
                        "thread before having timed out. This represents a "
                        "serious error and should be reported.");
  }
  return grpc::Status(grpc::StatusCode::ABORTED,
                      "The Transaction has already been committed or "
                      "rolled back.");
}

ReadTransaction::~ReadTransaction() {
  if (!done_) {
    // This really should not happen, and typically indicates a programming
    // error. TODO: Log this error and make it an alert.
    manager_.Release(tx_, read_start_time_);
  }
}

WriteTransaction::TrackingState::TrackingState() {}
WriteTransaction::TrackingState::~TrackingState() {}

WriteTransaction::WriteTransaction(TransactionManager& manager,
                                   WriteTransaction::TrackingState* state) :
    manager_(manager), state_(state), tx_(nullptr),
    write_start_time_(UINT64_C(0)), ops_(), completions_(),
    rollbacks_(), done_(false) {
  tx_ = manager.StartWriteTransaction(&write_start_time_);
}

WriteTransaction::WriteTransaction(
    TransactionManager& manager,
    WriteTransaction::TrackingState* state,
    Transaction* tx) :
    manager_(manager), state_(state), tx_(tx),
    write_start_time_(tx->operation_start_time()), ops_(), completions_(),
    rollbacks_(), done_(false) {}

void WriteTransaction::AddOperation(
    WriteTransaction::OpFunction op,
    WriteTransaction::CompletionFunction completion,
    WriteTransaction::RollbackFunction rollback) {
  VariantOpFunction var_op(op);
  VariantCompletionFunction var_completion(completion);
  VariantRollbackFunction var_rollback(rollback);
  ops_.push_back(var_op);
  completions_.push_back(var_completion);
  rollbacks_.push_back(var_rollback);
}

void WriteTransaction::AddOperation(
    WriteTransaction::TrackingOpFunction op,
    WriteTransaction::TrackingCompletionFunction completion,
    WriteTransaction::TrackingRollbackFunction rollback) {
  VariantOpFunction var_op(op);
  VariantCompletionFunction var_completion(completion);
  VariantRollbackFunction var_rollback(rollback);
  ops_.push_back(var_op);
  completions_.push_back(var_completion);
  rollbacks_.push_back(var_rollback);
}

WriteTransaction::~WriteTransaction() {
  if (!done_) {
    // TODO: Log an error if we reach here.
    manager_.Release(tx_, write_start_time_);
  }
}

grpc::Status WriteTransaction::HandleFailDuringCommit(
    const grpc::Status& fail_result,
    uint64_t timeout,
    uint64_t current_nanos,
    int failed_op_index) {
  grpc::Status ret_val;
  bool requires_release = true;
  bool rollback_result = tx_->Rollback(write_start_time_);
  if (!rollback_result) {
    requires_release = false;
    if (current_nanos > timeout) {
      if (fail_result.error_code() == grpc::StatusCode::DEADLINE_EXCEEDED) {
        ret_val = fail_result;
      } else {
        std::stringstream error;
        error << "Base error reported was ["
              << fail_result.error_code() << "] with message \""
              << fail_result.error_message()
              << "\" However, in addition, we timed out of the transaction, "
              << " and the transaction has sine been recovered for re-use.";
        ret_val = grpc::Status(grpc::StatusCode::DEADLINE_EXCEEDED,
                               error.str());
      }
    } else {
      std::stringstream error;
      error << "Base error reported was ["
            << fail_result.error_code() << "] with message \""
            << fail_result.error_message()
            << "\" However, in addition it seems that we are unable to "
            << "set the transaction state to rollback - which implies "
            << "that a separate thread modified the transaction state "
            << "before timeout. This is a serious error and should be "
            << "reported.";
      ret_val = grpc::Status(grpc::StatusCode::INTERNAL, error.str());
    }
  } else {
    ret_val = fail_result;
  }

  for (int j = failed_op_index-1; j >= 0; j--) {
    rollbacks_[j].use_tracking ?
        rollbacks_[j].tracking_rollback(tx_, state_, write_start_time_) :
        rollbacks_[j].rollback(tx_, write_start_time_);
  }
  if (requires_release) {
    manager_.Release(tx_, write_start_time_);
  }
  done_ = true;
  return ret_val;
}

grpc::Status WriteTransaction::HandleTimeoutDuringCommit(
    int last_success_index) {
  grpc::Status result = grpc::Status(grpc::StatusCode::DEADLINE_EXCEEDED,
                                     "Transaction timed out.");
  for (int j = last_success_index; j >= 0; j--) {
    rollbacks_[j].use_tracking ?
        rollbacks_[j].tracking_rollback(tx_, state_, write_start_time_) :
        rollbacks_[j].rollback(tx_, write_start_time_);
  }
  manager_.Release(tx_, write_start_time_);
  done_ = true;
  return result;
}

grpc::Status WriteTransaction::Commit() {
  if (done_) {
    return grpc::Status(grpc::StatusCode::ABORTED,
                        "Transaction already committed/rolled back.");
  }
  if (tx_->state() != Transaction::WRITE) {
    return grpc::Status(grpc::StatusCode::ABORTED,
                        "Transaction not in WRITE state.");
  }
  if (tx_->operation_start_time() != write_start_time_) {
    return grpc::Status(grpc::StatusCode::ABORTED,
        "Transaction has been re-purposed before call to commit.");
  }

  grpc::Status result;
  uint64_t timeout = write_start_time_ + manager_.get_timeout_nanos();

  for (uint16_t i = 0; i < ops_.size(); i++) {
    result = ops_[i].use_tracking ? ops_[i].tracking_op(tx_, state_)
                                  : ops_[i].op(tx_);
    uint64_t current_nanos = acumio::time::TimerNanosSinceEpoch();
    if (! result.ok()) {
      return HandleFailDuringCommit(result, timeout, current_nanos, i);
    }
    if (current_nanos >= timeout) {
      return HandleTimeoutDuringCommit(i);
    }
  }

  if (!tx_->StartWriteComplete(write_start_time_)) {
    for (int j = ops_.size() - 1; j >= 0; j--) {
      rollbacks_[j].use_tracking ?
          rollbacks_[j].tracking_rollback(tx_, state_, write_start_time_) :
          rollbacks_[j].rollback(tx_, write_start_time_);
    }
    manager_.Release(tx_, write_start_time_);
    std::stringstream error;
    error << "Unable to transition from WRITE state to COMPLETING_WRITE."
          << " The transaction appears to have timed out and been repurposed.";
    return grpc::Status(grpc::StatusCode::ABORTED, error.str());
  }

  for (uint16_t i = 0; i < ops_.size(); i++) {
    if (completions_[i].use_tracking) {
      completions_[i].tracking_completion(tx_, state_, write_start_time_);
    } else {
      completions_[i].completion(tx_, write_start_time_);
    }
  }

  manager_.Release(tx_, write_start_time_);
  return grpc::Status::OK;
}

bool WriteTransaction::Release() {
  if (!done_) {
    done_ = true;
    return manager_.Release(tx_, write_start_time_);
  }
  return false;
}

TransactionManager::TransactionManager(
    uint16_t pool_size, uint64_t timeout_nanos, uint64_t reap_timeout_nanos,
    const acumio::test::TestHook<Transaction*>* hook) :
    transaction_pool_(new Transaction*[pool_size]), pool_size_(pool_size),
    timeout_nanos_(timeout_nanos), reap_timeout_nanos_(reap_timeout_nanos),
    hook_(hook), free_list_(), age_list_() {
  // assert(timeout_nanos < reap_timeout_nanos)
  for (Transaction::Id i = 0; i < pool_size_; i++) {
    transaction_pool_[i] = new Transaction(hook, i);
  }
  // The next transaction to be allocated will be transaction 0.
  free_list_.push_back(0);
}

TransactionManager::~TransactionManager() {
  for (uint16_t i = 0; i < pool_size_; i++) {
    delete transaction_pool_[i];
  }

  delete[] transaction_pool_;
}

Transaction* TransactionManager::StartReadTransaction(uint64_t* start_time) {
  Transaction* ret_val = AcquireTransaction();
  *start_time = acumio::time::TimerNanosSinceEpoch();
  if (!ret_val->BeginRead(*start_time)) {
    return nullptr;
  }
  return ret_val;
}

Transaction* TransactionManager::StartWriteTransaction(uint64_t* start_time) {
  Transaction* ret_val = AcquireTransaction();
  *start_time = acumio::time::TimerNanosSinceEpoch();
  if (!ret_val->BeginWrite(*start_time)) {
    return nullptr;
  }
  return ret_val;
}

Transaction* TransactionManager::AcquireTransaction() {
  std::lock_guard<std::mutex> guard(state_guard_);
  UnguardedReleaseOldTransactions();
  Transaction::Id return_index = free_list_.back();
  free_list_.pop_back();
  age_list_.push_back(return_index);
  if (free_list_.empty()) {
    free_list_.push_back(age_list_.size());
  }
  if (return_index == pool_size_) {
    // This is actually not a great position to be in: It suggests that
    // the transaction pool was initially made too small. Notice that this
    // is one of the reasons we use Transaction* values instead of
    // returning a Transaction reference. If it were a Transaction reference,
    // the reference would need to be moved when we reach here, and that's just
    // a messy process.
    // TODO: Alert if we reach here.
    // TODO: Serious Alert if pool_size_ would grow to more than 32k
    //       At that point, the next time we grow, we break.
    pool_size_ *= 2;
    Transaction** new_transaction_pool = new Transaction*[pool_size_];
    for (Transaction::Id i = 0, j = return_index; i < return_index; i++, j++) {
      // Taking advantage of pipelining.
      new_transaction_pool[i] = transaction_pool_[i];
      new_transaction_pool[j] = new Transaction(hook_, j);
    }
    delete [] transaction_pool_;
    transaction_pool_ = new_transaction_pool;
  }
  return transaction_pool_[return_index];
}

bool TransactionManager::Release(Transaction* tx, uint64_t expected_op_time) {
  std::lock_guard<std::mutex> guard(state_guard_);
  if (!tx->Reset(expected_op_time)) {
    return false;
  }
  Transaction::Id tx_id = tx->id();
  free_list_.push_back(tx_id);
  for (auto it = age_list_.begin(); it < age_list_.end(); it++) {
    if (*it == tx_id) {
      age_list_.erase(it);
      break;
    }
  }
  return true;
}

void TransactionManager::ReleaseOldTransactions() {
  std::lock_guard<std::mutex> guard(state_guard_);
  UnguardedReleaseOldTransactions();
}

void TransactionManager::UnguardedReleaseOldTransactions() {
  // Caller should already have state_guard_ locked.
  uint64_t reaper_time = acumio::time::LatestTimeoutTime(reap_timeout_nanos_);
  Transaction::Id age_index = 0;
  while (age_index < age_list_.size()) {
    Transaction::Id tx_id = age_list_[age_index];
    Transaction* tx_to_check = transaction_pool_[tx_id];
    if(tx_to_check->ResetIfOld(reaper_time)) {
      free_list_.push_back(tx_id);
      age_index++;
    } else {
      break;
    }
  }
  auto begin_it = age_list_.begin();
  auto end_it = begin_it + age_index;
  age_list_.erase(begin_it, end_it);
}

} // namespace transaction
} // namespace acumio

