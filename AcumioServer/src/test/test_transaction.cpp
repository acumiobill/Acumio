//============================================================================
// Name        : test_comparable.cpp
// Author      : Bill Province (bill@acumio.com)
// Version     :
// Copyright   : Copyright (C) 2016 Acumio
// Description : A test_driver for our Transaction classes. This includes
//               Transaction, TransactionManager, ReadTransaction and
//               WriteTransaction.
//============================================================================
#include "transaction.h"
#include <functional>
#include <gtest/gtest.h>
#include <iostream> // Remove me expect when debugging. needed for std::cout.
#include <mutex>
#include <vector>

#include "gtest_extensions.h"
#include "test_hooks.h"
#include "time_util.h"

namespace acumio {
namespace {
const uint64_t one_second = acumio::time::NANOS_PER_SECOND;
const uint64_t one_millisecond = acumio::time::NANOS_PER_MILLI;
const uint64_t one_microsecond = acumio::time::NANOS_PER_MICRO;
using acumio::transaction::ReadTransaction;
using acumio::transaction::Transaction;
using acumio::transaction::TransactionManager;
using acumio::transaction::WriteTransaction;

// The following class demonstrates the spirit of a "transaction-aware"
// class. It versions its data elements, and builds in the concepts
// directly into the APIs. In addition, it handles TransactionManager
// and Transaction parameters, and knows how to get information about
// the Transactions to handle modification and cleanup and rollback
// tasks. Sadly, this looks rather complicated, and to tell the truth,
// it is: Any single value needs to become a versioned value with all the
// implied complexity.
//
// The class is written using mutexes instead of "lock-free" operations.
// Since we are really testing our Transaction classes rather than the
// notion of a Transaciton-aware class, this is fine.
class TestTxAware {
 public:
  struct Version {
    uint32_t value;
    uint64_t create_time;
    uint64_t remove_time;
  };

  struct EditVersion {
    const Transaction* tx;
    // The following attribute should match the operation_time of tx
    // used to create this edit version.
    uint64_t edit_start_time;
    uint32_t value;
    bool active; // If false, ignore the EditVersion information.
  };

  TestTxAware(uint64_t cleanup_nanos, uint32_t initial_value) :
    TestTxAware(cleanup_nanos, initial_value,
                acumio::time::TimerNanosSinceEpoch()) {}

  TestTxAware(uint64_t cleanup_nanos, uint32_t initial_value,
              uint64_t create_time) :
    cleanup_nanos_(cleanup_nanos), versioned_items_(), edit_version_() {
    Version elt;
    elt.value = initial_value;
    elt.create_time = create_time;
    elt.remove_time = acumio::time::END_OF_TIME;
    versioned_items_.push_back(elt);
  }

  ~TestTxAware() {}

  grpc::Status SetValue(uint32_t new_value, const Transaction* tx) {
    if (new_value == 17) {
      // Okay, this is kind of fake. However, it is possible for some
      // values to be rejected based on various conditions, and we
      // should account for that.
      return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "");
    }
    Transaction::AtomicInfo tx_new_info = tx->GetAtomicInfo();
    if (tx_new_info.state != Transaction::WRITE) {
      return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "");
    }

    std::lock_guard<std::mutex> edit_lock(edit_guard_);
    if (edit_version_.active) {
      uint64_t prior_commit_time;
      Transaction::AtomicInfo edit_info;
      edit_version_.tx->GetAtomicInfo(&edit_info, &prior_commit_time);
      bool cleanup_timeout = tx_new_info.operation_start_time >=
          edit_info.operation_start_time + cleanup_nanos_;
      if (cleanup_timeout) {
        SetEditVersion(tx, tx_new_info.operation_start_time, new_value);
        return grpc::Status::OK;
      }
      if (edit_version_.edit_start_time == edit_info.operation_start_time) {
        switch (edit_info.state) {
          case Transaction::WRITE:
            return grpc::Status(grpc::StatusCode::ABORTED, "concurrency error");
          case Transaction::COMPLETING_WRITE: {
            std::lock_guard<std::mutex> versions_lock(versions_guard_);
            PushEditVersion(prior_commit_time);
            SetEditVersion(tx, tx_new_info.operation_start_time,
                           new_value);
            return grpc::Status::OK;
          }
          default: 
            return grpc::Status(grpc::StatusCode::INTERNAL, "bad stuff");
        }
      }
      // If we reach here, the start_time listed on edit_version_ did not
      // match the operation_start_time for the transaction listed on the
      // edit version. In which case, we treat this just as if the edit_version
      // was inactive - i.e., we co-opt the Edit version with our new update.
    }
    SetEditVersion(tx, tx_new_info.operation_start_time, new_value);
    return grpc::Status::OK;
  }

  grpc::Status GetValue(const Transaction* tx, uint32_t* value) {
    Transaction::AtomicInfo info = tx->GetAtomicInfo();
    if (info.state != Transaction::READ) {
      return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "");
    }
    { // code-block to limit duration of edit_lock.
      std::lock_guard<std::mutex> edit_lock(edit_guard_);
      if (edit_version_.active) {
        if (edit_version_.edit_start_time < info.operation_start_time) {
          Transaction::AtomicInfo edit_info = edit_version_.tx->GetAtomicInfo();
          if (edit_info.state == Transaction::COMPLETING_WRITE) {
            *value = edit_version_.value;
            return grpc::Status::OK;
          }
        }
      }
    }

    std::lock_guard<std::mutex> version_lock(versions_guard_);
    int32_t version_ndx = versioned_items_.size() - 1;
    Version& current_version = versioned_items_[version_ndx];
    while (info.operation_start_time < current_version.create_time) {
      version_ndx--;
      if (version_ndx < 0) {
        return grpc::Status(grpc::StatusCode::OUT_OF_RANGE,
                            "No version matching time period.");
      }
      current_version = versioned_items_[version_ndx];
    }
    *value = current_version.value;
    return grpc::Status::OK;
  }

  void CompleteWriteOperation(const Transaction* tx, uint64_t write_start) {
    uint64_t complete_time;
    Transaction::AtomicInfo tx_info;
    tx->GetAtomicInfo(&tx_info, &complete_time);
    
    std::lock_guard<std::mutex> edit_lock(edit_guard_);
    if (edit_version_.edit_start_time == write_start) {
      std::lock_guard<std::mutex> version_lock(versions_guard_);
      PushEditVersion(complete_time);
      ClearEditVersion();
    }
  }

  void RollbackEdit(const Transaction* tx, uint64_t write_start) {
    uint64_t complete_time;
    Transaction::AtomicInfo tx_info;
    tx->GetAtomicInfo(&tx_info, &complete_time);
    
    std::lock_guard<std::mutex> edit_lock(edit_guard_);
    if (edit_version_.edit_start_time == write_start) {
      ClearEditVersion();
    }
  }

  uint32_t VersionCountUnsafe() const {
    return versioned_items_.size();
  }

  uint32_t IthVersionValueUnsafe(uint32_t i) const {
    return versioned_items_[i].value;
  }

 private:
  // Assumes edit_guard_ already locked.
  void SetEditVersion(const Transaction* tx, uint64_t edit_start_time,
                      uint32_t value) {
    edit_version_.tx = tx;
    edit_version_.edit_start_time = edit_start_time;
    edit_version_.value = value;
    edit_version_.active = true;
  }

  // Assumes edit_guard_ already locked.
  void ClearEditVersion() {
    edit_version_.tx = nullptr;
    edit_version_.edit_start_time = UINT64_C(0);
    edit_version_.value = UINT32_C(0);
    edit_version_.active = false;
  }

  // Assumes versions_guard_ already locked.
  void PushEditVersion(uint64_t commit_time) {
    Version elt;
    elt.value = edit_version_.value;
    elt.create_time = commit_time;
    elt.remove_time = acumio::time::END_OF_TIME;
    versioned_items_[versioned_items_.size() - 1].remove_time = commit_time;
    versioned_items_.push_back(elt);
  }

  uint64_t cleanup_nanos_;
  // Shared locks might be more appropriate, but we are really testing the
  // Transaction class here, not the general notion of 'Transaction-Awareness'.
  std::mutex versions_guard_;
  std::vector<Version> versioned_items_;
  std::mutex edit_guard_;
  EditVersion edit_version_;
};

WriteTransaction::OpFunction SetValueCallback(TestTxAware& item,
                                              uint32_t value) {
  return std::bind(&TestTxAware::SetValue,
                   &item,
                   value,
                   std::placeholders::_1);
}

grpc::Status TxSleepNanos(const Transaction* ignore,
                          uint64_t nanos) {
  acumio::time::SleepNanos(nanos);
  return grpc::Status::OK;
}

void TxNoOp(const Transaction* ignore, uint64_t also_ignore) {
  return;
}

WriteTransaction::OpFunction SleepNoOpCallback(uint64_t nanos) {
  return std::bind(TxSleepNanos,
                   std::placeholders::_1,
                   nanos);
}

WriteTransaction::CompletionFunction CompleteWriteCallback(TestTxAware& item) {
  return std::bind(&TestTxAware::CompleteWriteOperation,
                   &item,
                   std::placeholders::_1,
                   std::placeholders::_2);
}

WriteTransaction::CompletionFunction CompleteNoOpCallback() {
  return std::bind(TxNoOp, std::placeholders::_1, std::placeholders::_2);
}

WriteTransaction::RollbackFunction RollbackCallback(TestTxAware& item) {
  return std::bind(&TestTxAware::RollbackEdit,
                   &item,
                   std::placeholders::_1,
                   std::placeholders::_2);
}

WriteTransaction::RollbackFunction RollbackNoOpCallback() {
  return std::bind(TxNoOp, std::placeholders::_1, std::placeholders::_2);
}

class SleepyTxTestHook : public acumio::test::TestHook<Transaction*> {
 public:
  SleepyTxTestHook() : TestHook(), sleep_1(0), tx_1(0) {}
  ~SleepyTxTestHook() {}

  inline virtual void invoke_1(Transaction* tx) {
    if (sleep_1 > 0 && tx != nullptr && tx->id() == tx_1) {
      acumio::time::SleepNanos(sleep_1);
    }
  }

  void set_invoke_1(uint64_t nanos, uint16_t tx_id) {
    sleep_1 = nanos;
    tx_1 = tx_id;
  }

 private:
  uint64_t sleep_1;
  uint16_t tx_1;
};

TEST(TransactionTest, TransactionStateDiagram) {
  // In a real installation, we will want to have these numbers be
  // configurable via an .ini file of some sort. However, these are
  // pretty reasonable as a starting point as long as we are doing
  // all operations in memory.
  // In a "real" machine, the number 32 is a great number because it
  // means that the transaction pool indices will be represented in
  // a single line of L1 cache. Moreover, for a 4-CPU system, having
  // 16 pending transactions is actually quite a lot. Once we start
  // supporting persistence however, we should probably adjust these
  // numbers.
  // 16: Initial Transaction Pool Size.
  // one_second: timeout.
  // 2 seconds: "reap" timeout. Basically, how long to wait until we
  // give up on the original transaction figuring out that it should
  // clean up its mess.
  acumio::test::TestHook<Transaction*> hook;
  TransactionManager manager(16, one_second, 2 * one_second, &hook);
  uint64_t first_start_time;
  Transaction* tx = manager.StartReadTransaction(&first_start_time);
  ASSERT_NE(tx, nullptr);
  EXPECT_EQ(tx->state(), Transaction::READ);
  EXPECT_EQ(tx->id(), 0);
  uint64_t read_time = tx->operation_start_time();
  // No concurrent threads acting on this, so this should be a safe bet.
  EXPECT_EQ(first_start_time, read_time);
  ASSERT_TRUE(tx->Commit(read_time));
  EXPECT_EQ(tx->state(), Transaction::COMMITTED);
  manager.Release(tx, first_start_time);
  EXPECT_EQ(tx->state(), Transaction::NOT_STARTED);
  uint64_t second_start_time;
  tx = manager.StartReadTransaction(&second_start_time);
  EXPECT_EQ(tx->state(), Transaction::READ);
  EXPECT_EQ(tx->id(), 0);
  read_time = tx->operation_start_time();
  ASSERT_TRUE(tx->Rollback(read_time));
  EXPECT_EQ(tx->state(), Transaction::ROLLED_BACK);
  manager.Release(tx, read_time);
  EXPECT_EQ(tx->state(), Transaction::NOT_STARTED);
  uint64_t third_start_time;
  tx = manager.StartWriteTransaction(&third_start_time);
  EXPECT_EQ(tx->state(), Transaction::WRITE);
  EXPECT_EQ(tx->id(), 0);
  uint64_t write_time = tx->operation_start_time();
  ASSERT_TRUE(tx->StartWriteComplete(write_time));
  EXPECT_EQ(tx->state(), Transaction::COMPLETING_WRITE);
  uint64_t updated_write_time = tx->operation_complete_time();
  // This test is slightly suspect, because we are not forcing a wait.
  // So if the computer's "steady" clock has weak granularity, this
  // test may fail, as the updated_write_time and write_time might read
  // as the same values. A bit of measurements reveal the differences
  // for these values to be in the range of 300~1800 nanoseconds.
  // So a clock with microsecond granularity might not be able to tell
  // the difference. TODO: Consider putting in a sleep delay for a
  // millisecond to address this. So far, on my laptop, this is not
  // an issue.
  EXPECT_GT(updated_write_time, write_time);
  ASSERT_TRUE(tx->Commit(write_time));
  EXPECT_EQ(tx->state(), Transaction::COMMITTED);
  EXPECT_EQ(updated_write_time, tx->operation_complete_time());
  manager.Release(tx, write_time);
  EXPECT_EQ(tx->state(), Transaction::NOT_STARTED);
  uint64_t another_write_time;
  tx = manager.StartWriteTransaction(&another_write_time);
  EXPECT_EQ(tx->state(), Transaction::WRITE);
  EXPECT_EQ(tx->id(), 0);
  write_time = tx->operation_start_time();
  ASSERT_TRUE(tx->Rollback(write_time));
  EXPECT_EQ(tx->state(), Transaction::ROLLED_BACK);
  EXPECT_EQ(tx->operation_start_time(), write_time);
}

TEST(TransactionTest, SimpleReadTransactionTest) {
  acumio::test::TestHook<Transaction*> hook;
  TransactionManager manager(16, one_second, 2 * one_second, &hook);
  ReadTransaction tx(manager);
  EXPECT_OK(tx.Commit());
}

TEST(TransactionTest, CompareExchangeTest) {
  std::atomic<Transaction::AtomicInfo> val;
  Transaction::AtomicInfo initial_val;
  initial_val.state = Transaction::NOT_STARTED;
  initial_val.operation_start_time = UINT64_C(0);
  val.store(initial_val);
  Transaction::AtomicInfo expected_val;
  expected_val.state = Transaction::NOT_STARTED;
  expected_val.operation_start_time = UINT64_C(0);
  Transaction::AtomicInfo update_val;
  update_val.state = Transaction::READ;
  update_val.operation_start_time = acumio::time::TimerNanosSinceEpoch();
  EXPECT_TRUE(val.compare_exchange_strong(expected_val, update_val,
                                          std::memory_order_relaxed));
}

TEST(TransactionTest, NoOpWriteTransactionTest) {
  acumio::test::TestHook<Transaction*> hook;
  TransactionManager manager(16, one_second, 2 * one_second, &hook);
  WriteTransaction tx(manager);
  EXPECT_OK(tx.Commit());
  WriteTransaction tx2(manager);
  EXPECT_OK(tx2.Commit());
}

TEST(TransactionTest, SimpleWriteTransactionTest) {
  acumio::test::TestHook<Transaction*> hook;
  TransactionManager manager(16, one_second, 2 * one_second, &hook);
  TestTxAware item1(manager.get_reap_timeout_nanos(), 0);
  TestTxAware item2(manager.get_reap_timeout_nanos(), 0);
  WriteTransaction tx(manager);
  tx.AddOperation(SetValueCallback(item1, 1),
                  CompleteWriteCallback(item1),
                  RollbackCallback(item1));
  tx.AddOperation(SetValueCallback(item2, 2),
                  CompleteWriteCallback(item2),
                  RollbackCallback(item2));
  EXPECT_OK(tx.Commit());
  ASSERT_EQ(2, item1.VersionCountUnsafe());
  EXPECT_EQ(0, item1.IthVersionValueUnsafe(0));
  EXPECT_EQ(1, item1.IthVersionValueUnsafe(1));
  ASSERT_EQ(2, item2.VersionCountUnsafe());
  EXPECT_EQ(0, item2.IthVersionValueUnsafe(0));
  EXPECT_EQ(2, item2.IthVersionValueUnsafe(1));
}

TEST(TransactionTest, WriteTransactionRollbackTest) {
  acumio::test::TestHook<Transaction*> hook;
  TransactionManager manager(16, one_second, 2 * one_second, &hook);
  TestTxAware item1(manager.get_reap_timeout_nanos(), 0);
  TestTxAware item2(manager.get_reap_timeout_nanos(), 0);
  WriteTransaction tx(manager);
  tx.AddOperation(SetValueCallback(item1, 1),
                  CompleteWriteCallback(item1),
                  RollbackCallback(item1));
  // Setting it to the value 17 should cause the transaction to rollback -
  // therefore, we will revert item 1 back to its original state of 1 item
  // as well.
  tx.AddOperation(SetValueCallback(item2, 17),
                  CompleteWriteCallback(item2),
                  RollbackCallback(item2));
  EXPECT_ERROR(tx.Commit(), grpc::StatusCode::INVALID_ARGUMENT);
  ASSERT_EQ(1, item1.VersionCountUnsafe());
  EXPECT_EQ(0, item1.IthVersionValueUnsafe(0));
  ASSERT_EQ(1, item2.VersionCountUnsafe());
  EXPECT_EQ(0, item2.IthVersionValueUnsafe(0));
}

TEST(TransactionTest, TransactionFloodTest) {
  acumio::test::TestHook<Transaction*> hook;
  TransactionManager manager(16, one_second, 2 * one_second, &hook);
  // 18 is chosen so that we expand past our initial Transaction pool size.
  const uint32_t tx_count = 18;
  uint64_t tx_times[tx_count];
  Transaction* transactions[tx_count];
  for (uint16_t i = 0; i < tx_count; i++) {
    tx_times[i] = 0;
    transactions[i] = nullptr;
    transactions[i] = manager.StartReadTransaction(&(tx_times[i]));
  }

  for (uint16_t i = 0; i < tx_count; i++) {
    EXPECT_EQ(transactions[i]->id(), i);
    EXPECT_EQ(transactions[i]->operation_start_time(), tx_times[i]);
  }

  for (uint16_t i = 0; i < tx_count; i++) {
    EXPECT_TRUE(transactions[i]->Commit(tx_times[i]));
  }

  for (uint16_t i = 0; i < tx_count; i++) {
    EXPECT_TRUE(manager.Release(transactions[i], tx_times[i]));
  }
}

TEST(TransactionTest, TransactionTimeoutTest) {
  acumio::test::TestHook<Transaction*> hook;
  TransactionManager manager(16, 100 *one_microsecond, 200 * one_microsecond,
                             &hook);
  TestTxAware item1(manager.get_reap_timeout_nanos(), 0);
  TestTxAware item2(manager.get_reap_timeout_nanos(), 0);
  WriteTransaction tx(manager);
  tx.AddOperation(SleepNoOpCallback(300 * one_microsecond),
                  CompleteNoOpCallback(),
                  RollbackNoOpCallback());
  tx.AddOperation(SetValueCallback(item1, 1),
                  CompleteWriteCallback(item1),
                  RollbackCallback(item1));
  tx.AddOperation(SetValueCallback(item2, 2),
                  CompleteWriteCallback(item2),
                  RollbackCallback(item2));
  EXPECT_ERROR(tx.Commit(), grpc::StatusCode::DEADLINE_EXCEEDED);
  ASSERT_EQ(1, item1.VersionCountUnsafe());
  EXPECT_EQ(0, item1.IthVersionValueUnsafe(0));
  ASSERT_EQ(1, item2.VersionCountUnsafe());
  EXPECT_EQ(0, item2.IthVersionValueUnsafe(0));
  WriteTransaction tx2(manager);
  tx2.AddOperation(SetValueCallback(item1, 1),
                   CompleteWriteCallback(item1),
                   RollbackCallback(item1));
  tx2.AddOperation(SleepNoOpCallback(300 * one_microsecond),
                   CompleteNoOpCallback(),
                   RollbackNoOpCallback());
  tx2.AddOperation(SetValueCallback(item2, 2),
                   CompleteWriteCallback(item2),
                   RollbackCallback(item2));
  EXPECT_ERROR(tx2.Commit(), grpc::StatusCode::DEADLINE_EXCEEDED);
  ASSERT_EQ(1, item1.VersionCountUnsafe());
  EXPECT_EQ(0, item1.IthVersionValueUnsafe(0));
  ASSERT_EQ(1, item2.VersionCountUnsafe());
  EXPECT_EQ(0, item2.IthVersionValueUnsafe(0));
  WriteTransaction tx3(manager);
  tx3.AddOperation(SetValueCallback(item1, 1),
                   CompleteWriteCallback(item1),
                   RollbackCallback(item1));
  tx3.AddOperation(SetValueCallback(item2, 2),
                   CompleteWriteCallback(item2),
                   RollbackCallback(item2));
  tx3.AddOperation(SleepNoOpCallback(300 * one_microsecond),
                   CompleteNoOpCallback(),
                   RollbackNoOpCallback());
  EXPECT_ERROR(tx3.Commit(), grpc::StatusCode::DEADLINE_EXCEEDED);
  ASSERT_EQ(1, item1.VersionCountUnsafe());
  EXPECT_EQ(0, item1.IthVersionValueUnsafe(0));
  ASSERT_EQ(1, item2.VersionCountUnsafe());
  EXPECT_EQ(0, item2.IthVersionValueUnsafe(0));
  WriteTransaction tx4(manager);
  tx4.AddOperation(SetValueCallback(item1, 1),
                   CompleteWriteCallback(item1),
                   RollbackCallback(item1));
  tx4.AddOperation(SetValueCallback(item2, 2),
                   CompleteWriteCallback(item2),
                   RollbackCallback(item2));
  EXPECT_OK(tx4.Commit());
  ASSERT_EQ(2, item1.VersionCountUnsafe());
  EXPECT_EQ(0, item1.IthVersionValueUnsafe(0));
  EXPECT_EQ(1, item1.IthVersionValueUnsafe(1));
  ASSERT_EQ(2, item2.VersionCountUnsafe());
  EXPECT_EQ(0, item2.IthVersionValueUnsafe(0));
  EXPECT_EQ(2, item2.IthVersionValueUnsafe(1));
}

TEST(TransactionTest, BadTimingTests) {
  SleepyTxTestHook hook;
  TransactionManager manager(16, 100 *one_microsecond, 200 * one_microsecond,
                             &hook);
  TestTxAware item1(manager.get_reap_timeout_nanos(), 0);
  TestTxAware item2(manager.get_reap_timeout_nanos(), 0);
  hook.set_invoke_1(101 * one_microsecond, 0);
  hook.set_invoke_1(400 * one_microsecond, 1);
  ReadTransaction tx(manager);
  ReadTransaction tx2(manager);
  EXPECT_OK(tx.Commit());
  acumio::time::SleepNanos(300 * one_microsecond);
  WriteTransaction tx3(manager);
  tx3.AddOperation(SetValueCallback(item1, 1),
                   CompleteWriteCallback(item1),
                   RollbackCallback(item1));
  tx3.AddOperation(SetValueCallback(item2, 2),
                   CompleteWriteCallback(item2),
                   RollbackCallback(item2));
  EXPECT_ERROR(tx2.Commit(), grpc::StatusCode::DEADLINE_EXCEEDED);
  EXPECT_OK(tx3.Commit());
}

} // anonymous namespace
} // namespace acumio

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
