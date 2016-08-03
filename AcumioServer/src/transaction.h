#ifndef AcumioServer_transaction_h
#define AcumioServer_transaction_h
//============================================================================
// Name        : transaction.h
// Author      : Bill Province (bill@acumio.com)
// Version     :
// Copyright   : Copyright (C) 2016 Acumio
// Description : This file contains classes related to transaction management.
//               When working with this, you should have a single
//               TransactionManager instance that can be shared by multiple
//               threads.
//
//               The usual working pattern for this class is as follows for
//               mutation operations:
//                  grpc::Status SomeMethod(TransactionAwareClass c,
//                                          std::vector<ThingToDoToC> v) {
//                     // reference to manager by some context.
//                     WriteTransaction tx(manager_);
//                     for (int i = 0; i < v.size(); i++) {
//                       tx.AddOperation(c.DoSomethingMethodReference(v[i]),
//                                       c.FinishDoSomethingReference(v[i]),
//                                       c.RollbackSomethingReference(v[i]));
//                     }
//                     return tx.Commit();
//                  }
//
//               In the example above, the 'TransactionAwareClass' refers
//               to any class that properly deals with Transactions.
//
//               The TransactionManager is responsible for the lifecycle
//               of Transactions, including allocation and cleanup.
//
//               The Transaction class has current state information regarding
//               a transaction, but is totally unaware of the objects operated
//               on with the Transaction.
//
//               For a ReadTransaction example:
//               grpc::Status SomeReadMethod() {
//                 ReadTransaction tx(_manager);
//                 DoStuff(tx.get_read_start_time());
//                 tx.commit();
//               }
//
//               In the example above, the DoStuff(tx.get_transaction()) call
//               may optionally care about the time the transaction was
//               started, so that it can get a consistent view of the content
//               being read. When the transaction commits, we basically
//               forget about the transaction and return it to the
//               TransactionManager.
//
//               Our transaction model is as follows:
//
//               Premise 1:
//               For a read operation to succeed, it must obtain a consistent
//               view of data. Moreover, we will always provide a consistent
//               data view, even in the face of concurrent modifications.
//
//               If data is in the process of being modified by a concurrent
//               transaction, then the data that existed at the start of the
//               read operation must still be made available by the modifying
//               operation.
//
//               Note that with this premise, the important factor is the
//               data as it existed at the time of the read start operation.
//               We will always return data consistent with the data as it
//               existed then, but there is no guarantee that the data returned
//               is the most up-to-date version.
//
//               Premise 2:
//               For a write operation to succeed, it must only write to data
//               where it has a consistent view of the content. In the face of
//               concurrent modifications, the first editor of a piece of
//               data wins the race.
//
//               Therefore, if a write operation starts, and it encounters a
//               piece of data being modified by a concurrent transaction, we
//               will roll back the write operation, because we cannot
//               guarantee a consistent view.
//
//               The view-consistency must be for the entire duration of the
//               write operation: from the moment that the modifications begin
//               to the moment that the modification is complete.
//
//               Premise 3:
//               All transactions have a "timeout" value that suggests that
//               if the transaction is not completed by the time indicated by
//               the timeout, the changes should be viewed as rolled back.
//
//               The reason to have this premise is to address the problem of
//               "thread hanging" or "thread death", which could possibly
//               create locks which would never expire on their own. In
//               addition, if something just takes an inordinately long
//               amount of time, it should still not completely stall the
//               system and starve out all other changes.
//
//               Premise 4:
//               When modifying a collection of data elements within a single
//               transaction, the transition of state for those changes from
//               "in-flight modification" to being a committed change must
//               occur in a single atomic action.
//
//               It would not suffice, by this premise to complete the changes
//               and then iterate over the modifications and push some of them
//               to be complete while keeping others incomplete. Particularly
//               in view of premise 3, where we can end up with a race
//               condition where part of the change is committed, but the
//               rest is rolled back due to timeout.
//
//               Premise 5:
//               Transactions do not live forever. In a way, this is part
//               of Premise 3, but it also suggests that eventually, the
//               transaction gets de-allocated or garbage collected.
//               (Our actual model is a garbage-collection model, but it
//               is not a built-in premise). The implication is that if we
//               modify data elements to refer to transactions, the
//               references will eventually become stale.
//
//               In addition to these premises, we make a particular design
//               choice: we will manage the allocation and de-allocation
//               of Transactions using a TransactionManager and in addition,
//               we may re-purpose an existing transaction.
//
//               Since the TransactionManager may re-purpose a transaction,
//               we need to guard against a presumed dead process coming back
//               to life and attempting to continue its prior use of a
//               re-purposed transaction. To guard against that, we use a
//               timestamp mechanism: we set an operation time for each
//               Transaction, and when we go back to work on the Transaction,
//               we provide our expected operation time. If the expected time
//               does not match the transaction's operation time, we can
//               assume that the transaction has been re-purposed.
//
//               The timestamping process will work as long as the timeout
//               thresholds are greater than our time measurement grain.
//               Our time measurement grain is driven by the computer's
//               finest grain "steady" clock (see time_util.h), and worst
//               case, we can expect millisecond grains. This means that
//               our timeout values must exceed 1ms. in duration. Our APIs
//               track time in terms of nanoseconds, but the internal
//               clocks may only track time in a coarser grain.
//               ******************************************************
//               As a result of these premises and design choices, we can
//               infer that our data model must be one that supports
//               versioning of the content - even if its ephemeral versioning.
//               Note that the versioning support is not just to support a
//               single extra copy of the data, since we might have this
//               sequence with respect to a single piece of data:
//
//               Read Tx 1 starts at time t
//               Write Tx 1 starts at time t+1 and commits at t+2
//               Read Tx 2 starts at time t+3
//               Write Tx 2 starts at time t+4 and commits at t+5
//               Read Tx 3 starts at time t+6
//               Read Tx 1 commits at t+7
//               Read Tx 3 commits at t+8
//               Read Tx 2 commits at t+9
//
//               In this sequence, Read Tx 1 needs to read data as it existed
//               at time t and Read Tx 2 needs to read data as it existed at
//               time t+3 and Read Tx 3 needs data as it exists at time
//               t+6. Since all 3 Read transactions are concurrent, and they
//               all need separate views of the same piece of data, we need
//               all three to be available concurrently.
//
//               Another result of the premises is that there needs to be
//               a mechanism to rollback a partially written write operation.
//               Notice that using a versioning scheme for the data allows
//               this in a natural fashion.
//
//               As a result of premise 4, we internally model an additional
//               stage over the usual set of transaction stages - the
//               COMPLETING_WRITE state. The idea is that we want an atomic
//               piece of data to modify to transition the state of all
//               data changes. The natural way to do this is to modify
//               the transaction itself. This means that concurrent
//               reads to data modified by a given transaction
//               need to be able to determine which transaction performed
//               the change, and whether or not that transaction has finished
//               all of the modifications. In this way, a read operation can
//               know which version of the data it should be considering -
//               the prior version or the version being modified by the
//               write operation.
//
//               When we enter the COMPLETING_WRITE stage, the changes made
//               by the transaction are considered permanent. However, we
//               need to clean up any references to the transaction that we
//               may have left with the data. Only after doing the cleanup
//               operations do we transition to the COMMIT stage and then
//               finally release the transaction.
//
//               Technically, we do not need to model the COMPLETING_WRITE
//               state as a completely different state: we could instead just
//               say we are in the COMMIT state and then cleanup before
//               releasing the transaction. However, we choose to introduce
//               this state to make the operations transparent, and eventually,
//               for monitoring purposes, we may want to see how much time
//               is being spent in each state, and having this information
//               is useful for that purpose.
//
//               Note that all time measurements are provided in nanoseconds,
//               and absolute time values are given as the number of
//               nanoseconds since the epoch (Jan 1, 1970 12:00 am GMT). See
//               time_util.h
//
//               Race Conditions:
//                 There is a race condition that occurs between the time
//                 that you transition a transaction to the state
//                 COMPLETING_WRITE to the point when the TransactionManager
//                 determines that the transaction is not going to come back
//                 to life. It is critical that the CompletionFunctions be
//                 written in a manner where failure is virtually impossible
//                 and cannot take long to perform. It is also theoretically
//                 possible for the thread-scheduler to mess us up: that is,
//                 the scheduler could put the thread that is doing the
//                 completion functions to sleep and keep it asleep
//                 indefinitely. This however, is a sign that you probably
//                 have too many processes running on your machine, or that
//                 you have the reap_timeout value set incorrectly. The
//                 reap_timeout should be set to a value such that:
//                    1) You could stand the system waiting for the duration
//                       of the reap_timeout before getting the thread back.
//                    2) If an actual completion operation requires more than
//                       reap_timeout, then it is probably broken.
//============================================================================

#include <grpc++/support/status.h>
#include <atomic>
#include <cstdint>
#include <mutex>
#include <vector>
#include "test_hooks.h"
#include "time_util.h"

namespace acumio {
namespace transaction {

class TransactionManager;

class Transaction {
 public:
  typedef uint16_t Id;

  // Use this value to represent a transaction id corresponding to no
  // transactions.
  static const Id NOT_A_TX = UINT16_MAX;

  // While it might seem odd to store a value with a storage requirement of
  // 3 bits inside a 64-bit integer, we need this to make the atomic
  // operations happy.
  enum State : uint64_t {
    NOT_STARTED = 0,
    READ = 1,
    WRITE = 2,
    COMPLETING_WRITE = 3,
    COMMITTED= 4,
    ROLLED_BACK = 5
  };

  // We intentionally align this to fill a complete 16 bytes since otherwise,
  // the atomic compare_exchange_strong operations will become unhappy.
  // For this reason, the State enum is artificially pushed to require
  // 64 bits.
  struct AtomicInfo {
    uint64_t operation_start_time;
    State state;
  };

  // This method returns true if the provided expected_write_time matches
  // the Transaction's operation_start_time and if the Transaction's state is
  // WRITE. If these conditions are met, the Transaction is transitioned
  // to the COMPLETING_WRITE state, and the transaction acquires an
  // operation_complete_time that is non-zero, and is set to the current time
  // using a steady clock (see time_util.h).
  //
  // If the conditions are not met, this just returns false.
  //
  // See documentation in header for the rationale behind making the Commit
  // operation for writes a two-step process instead of one.
  //
  // It is critical that if there are steps to perform after
  // StartWriteComplete but before Commit that these steps are operations
  // with no failure mode! There is no turning back after start_write_complete.
  bool StartWriteComplete(uint64_t expected_write_time);

  // Before issuing Commit, the transaction should be in one of two stages:
  // either in READ state, or in COMPLETING_WRITE state. if invoked while
  // the transaction is in any other state (including already being committed),
  // it returns false. In addition, it will return false if the
  // expected_write_time does not match the Transaction's operation_start_time.
  //
  // See documentation in header for the rationale behind making the Commit
  // operation for writes a two-step process instead of one.
  //
  // The operation time is unaffected by this method.
  bool Commit(uint64_t expected_write_time);

  // Performs a rollback operation, which will put the Transaction into the
  // state ROLLED_BACK. However, it will only do so if the following conditions
  // are met:
  //   1) The operation_start_time of the Transaction must match
  //      expected_start_time.
  //   2) The transaction state must be either a READ or a WRITE state.
  // If those two conditions are met, we rollback the transaction. Note that
  // there is really no harm in rolling back an already-rolled back
  // Transaction, but this method will return false in that case.
  //
  // The reason to match the expected_start_time is to guard against a situation
  // where the transaction has been re-purposed after a timeout. This should
  // be a very rare event, but we should nevertheless guard against it.
  bool Rollback(uint64_t expected_start_time);

  inline State state() const {
    AtomicInfo tmp = info_;
    return tmp.state;
  }
  inline Id id() const { return id_; }
  inline uint64_t operation_start_time() const {
    AtomicInfo tmp = info_;
    return tmp.operation_start_time;
  }
  inline uint64_t operation_complete_time() const {
    return operation_complete_time_;
  }

  inline AtomicInfo GetAtomicInfo() const {
    return info_.load();
  }

  inline void GetAtomicInfo(AtomicInfo* base_info,
                            uint64_t* op_complete_time) const {
    *op_complete_time = operation_complete_time_;
    *base_info = info_.load();
    // When we get past this next loop, we will know that the returned
    // op_complete_time had the value that it now has before and
    // after we loaded the rest of the data content for the transaction.
    // Note that operation_complete_time_ is denoted as volatile, so that
    // the optimizer does not optimize-away this while-loop check.
    while (*op_complete_time != operation_complete_time_) {
      *op_complete_time = operation_complete_time_;
      *base_info = info_.load();
    }
  }

 private:
  friend class TransactionManager;
  // Note that the life-cycle of transactions is only made available via
  // the TransactionManager. No public construction/destruction.
  // The TestHook is not owned by the Transaction, and is expected to have
  // a lifetime exceeding that of the Transaction.
  Transaction(const acumio::test::TestHook<Transaction*>* hook, Id id);
  ~Transaction();
  bool Reset(uint64_t expected_start_time);
  // Reset if operation_time_ <= youngest_reset. Returns true if we actually
  // performed the Reset.
  bool ResetIfOld(uint64_t youngest_reset);
  bool BeginRead(uint64_t read_start_time);
  bool BeginWrite(uint64_t write_start_time);
  bool StartWriteComplete(uint64_t expected_write_time,
                          uint64_t effective_commit_time);

  const acumio::test::TestHook<Transaction*>* hook_;
  Id id_;
  // Note that GetAtomicInfo will get *both* of these items atomically.
  // We don't put them into a single std::atomic because std::atomic on
  // structs will not work with more than 128 bits.
  volatile uint64_t operation_complete_time_;
  std::atomic<AtomicInfo> info_;
};

// Use this to hold sets of Transaction* values. When comparing two
// pointers, we want to compare the Ids of the referenced Transactions.
struct TxPointerLess {
  inline bool operator()(const Transaction* left,
                         const Transaction* right) const {
    return (left == nullptr) ? (right != nullptr) :
           (right == nullptr ? false : left->id() < right->id());
  }
};

class ReadTransaction {
 public:
  ReadTransaction(TransactionManager& manager);
  ~ReadTransaction();

  // Note that there is no Rollback for a ReadTransaction, since the idea
  // really does not make sense. A ReadTransaction makes no changes to be
  // rolled back. However, we expect that if a ReadTransaction is used to
  // obtain read-locks on data items, that we release those read-locks before
  // performing the commit operation. It will be application-dependent on
  // how to make this happen.
  grpc::Status Commit();

  inline uint64_t read_start_time() { return read_start_time_; }

 private:
  TransactionManager& manager_;
  Transaction* tx_;
  uint64_t read_start_time_;
  bool done_;
};

class WriteTransaction {
 public:

  typedef std::function<grpc::Status(const Transaction*)> OpFunction;

  typedef std::function<void(const Transaction*)> CompletionFunction;

  typedef std::function<void(const Transaction*)> RollbackFunction;

  WriteTransaction(TransactionManager& manager);
  WriteTransaction(TransactionManager& manager,
                   Transaction* started_transaction);
  ~WriteTransaction();
  
  void AddOperation(OpFunction op,
                    CompletionFunction completion,
                    RollbackFunction rollback);
  grpc::Status Commit();
  bool Release();

 private:
  grpc::Status HandleFailDuringCommit(const grpc::Status& fail_result,
                                      uint64_t timeout,
                                      uint64_t current_nanos,
                                      int failed_op_index);
  grpc::Status HandleTimeoutDuringCommit(int last_success_index);

  TransactionManager& manager_;
  Transaction* tx_;
  uint64_t write_start_time_;
  std::vector<OpFunction> ops_;
  std::vector<CompletionFunction> completions_;
  std::vector<RollbackFunction> rollbacks_;
  bool done_;
};

class TransactionManager {
 public:
  TransactionManager(uint16_t pool_size,
                     uint64_t timeout_nanos,
                     uint64_t reap_timeout_nanos,
                     const acumio::test::TestHook<Transaction*>* hook
                     );
  ~TransactionManager();

  // Acquires a transaction respectively for a read transaction or a
  // write transaction. Used respectively by the ReadTransaction and
  // WriteTransaction classes. The start_time attributes get modified
  // to match the Transaction start times. In other words, for at least
  // a brief time, after: uint64_t t; tx = StartReadTransaction(&t); we
  // can assert: (tx->operation_time() == t).
  // However, there is no guarantee that the transaction might not be
  // modified before trying this assert.
  // Note that it is theoretically possible for the time required to
  // perform this operation exceeds the reap_timeout, in which case,
  // the attempt to retrieve the Transaction could fail. This will
  // return a nullptr in the event of failure.
  Transaction* StartReadTransaction(uint64_t* start_time);
  Transaction* StartWriteTransaction(uint64_t* start_time);

  // When performing edits for in-flight transactions, we might identify
  // the transaction in the edit contents by the transaction_id. This
  // method allows us to find the current state of the provided transaction.
  // Note that once the transaction gets committed (or rolled back), there
  // should no longer be valid information about the transaction. It is
  // expected that the rollback operations remove old Transaction traces,
  // and that any traces also get removed between the time of entering
  // COMPLETING_WRITE and before entering COMMIT stage. In the WriteTransaction
  // class, the CompletionFunctions provided by the AddOperation method are
  // expected to take care of this.
  const Transaction* get_transaction(Transaction::Id transaction_id) const;

  inline uint16_t get_pool_size() const { return pool_size_; }
  inline uint64_t get_timeout_nanos() const { return timeout_nanos_; }
  inline uint64_t get_reap_timeout_nanos() const {
    return reap_timeout_nanos_;
  }
  inline uint64_t LatestReapTimeout() const {
    return acumio::time::LatestTimeoutTime(reap_timeout_nanos_);
  }

  inline const acumio::test::TestHook<Transaction*> * hook() const {
    return hook_;
  }

  // This releases the provided transaction back to the TransactionManager.
  // It also will reset the provided transaction.
  bool Release(Transaction* tx, uint64_t expected_op_time);

  // Looks at all active transactions and forces them to be released back
  // to the pending state if their age is greater than reap_timeout_nanos.
  void ReleaseOldTransactions();

 private:
  // Provides a free transaction to the caller - used either for
  // StartReadTransaction or StartWriteTransaction - so that we can
  // decide which transaction to provide.
  Transaction* AcquireTransaction();

  // This operation is assumed to be invoked when we already have the
  // state_guard_ mutex locked. We invoke it this way so that we can
  // efficiently share code. It is otherwise the same as
  // ReleaseOldTransactions().
  void UnguardedReleaseOldTransactions();

  // TODO: Replace this with home-made exclusive mutex class.
  std::mutex state_guard_;
  Transaction** transaction_pool_;
  uint16_t pool_size_;
  uint64_t timeout_nanos_;
  uint64_t reap_timeout_nanos_;
  const acumio::test::TestHook<Transaction*> * hook_;

  // We use the free_list to determine which will be the next Transaction
  // to allocate. At initialization, this will be 0. When we allocate a
  // Transaction, we pop the transaction_id from the free_list_, and
  // use that. If the free_list_ becomes empty by this process, we push
  // onto the free_list_ the size of the age_list_. When we release a
  // Transaction, we remove that transaction from the age_list (this is
  // a linear search - so we expect the number of pending transactions
  // to be small so that linear searches are most effective), and add that
  // value to the free_list.
  //
  // At any one point, we maintain this invariant:
  // forall(Integer i: 0 <= i < free_list_.size() + age_list_.size()
  //        implies i in free_list_ or i in age_list_)
  // A bit less formally: If your free_list_ has 17 elements and your
  // age_list has 13 elements, then you should expect to see all of the
  // values 0,1,2, ... 29 scattered between the age_list_ and the
  // free_list_.
  std::vector<Transaction::Id> free_list_;
  std::vector<Transaction::Id> age_list_;
};

} // namespace transaction
} // namespace acumio

#endif // AcumioServer_transaction_h
