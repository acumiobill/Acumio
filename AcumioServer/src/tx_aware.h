#ifndef AcumioServer_tx_aware_h
#define AcumioServer_tx_aware_h
//============================================================================
// Name        : tx_aware.h
// Author      : Bill Province (bill@acumio.com)
// Version     :
// Copyright   : Copyright (C) 2016 Acumio
// Description : Provides templated transaction-aware access to a versioned
//               data structure.
//============================================================================

#include <stdint.h> // needed for UINT16_C
#include <vector>
#include "shared_mutex.h"
#include "time_util.h"
#include "transaction.h"

namespace acumio {
namespace transaction {

template <typename EltType>
class TxAware {
 public:
  enum EditState : uint64_t {
    NOT_EDITING = 0,
    SETTING = 1,
    REMOVING = 2
  };

  struct EditStateTime {
    EditStateTime() : state(NOT_EDITING), time(UINT64_C(0)) {}
    EditStateTime(const EditState& s, uint64_t t) : state(s), time(t) {}
    EditStateTime(const EditStateTime& o) : state(o.state), time(o.time) {}
    EditState state;
    uint64_t time;
  };

  struct TimeBoundary {
    TimeBoundary() : create(UINT64_C(0)), remove(UINT64_C(0)) {}
    TimeBoundary(uint64_t c, uint64_t r) : create(c), remove(r) {}
    TimeBoundary(const TimeBoundary& o) : create(o.create), remove(o.remove) {}
    uint64_t create;
    uint64_t remove;
  };

  struct Version {
    Version() : value(), times() {}
    Version(const EltType& v, const TimeBoundary& t) : value(v), times(t) {}
    Version(const Version& o) : value(o.value), times(o.times) {}
    EltType value;
    TimeBoundary times;
  };

  TxAware(EltType not_present_value, uint64_t aware_start_time) :
      current_value_(not_present_value),
      current_value_times_(0, aware_start_time),
      not_present_value_(not_present_value), edit_value_(not_present_value_),
      edit_state_time_(NOT_EDITING, acumio::time::END_OF_TIME),
      edit_tx_(nullptr), versions_start_(0),
      versions_next_(0), historical_versions_() {}
  ~TxAware() {}

  bool IsLatestVersionAtTime(uint64_t access_time) const {
    SharedLock read_lock(guard_);
    return current_value_times_.create <= access_time &&
           edit_state_time_.state == NOT_EDITING;
  }

  const EltType& Get(uint64_t access_time, bool* exists_at_time) const {
    // This extra level of scoping brackets is here to establish scope for
    // the SharedLock on the guard_.
    {
      SharedLock read_lock(guard_);
      if (current_value_times_.create <= access_time) {
        if (edit_state_time_.state != NOT_EDITING &&
            edit_state_time_.time <= access_time) {
          // In this case, we should potentially use the value that is the edit
          // value, but only if the transaction is in the COMPLETING_WRITE
          // state, and the edit_state.time matches the transaction's start
          // time AND if the access time is later than the transaction's
          // operation_complete_time().
          if (edit_tx_->operation_complete_time() < access_time) {
            Transaction::AtomicInfo tx_info = edit_tx_->GetAtomicInfo();
            if (edit_state_time_.time == tx_info.operation_start_time &&
                tx_info.state == Transaction::COMPLETING_WRITE) {
              *exists_at_time = (edit_state_time_.state == SETTING);
              return edit_value_;
            }
          }
        }
        *exists_at_time = (access_time < current_value_times_.remove);
        return (*exists_at_time ? current_value_ : not_present_value_);
      }
    } // End: Extra scoping for mutex.
    return VersionSearch(access_time, exists_at_time);
  }

  grpc::Status Set(const EltType& e, const Transaction* tx,
                   uint64_t edit_time) {
    ExclusiveLock guard(guard_);
    Transaction::AtomicInfo current_tx_info = tx->GetAtomicInfo();
    if (current_tx_info.operation_start_time != edit_time) {
      return grpc::Status(grpc::StatusCode::DEADLINE_EXCEEDED,
        "The transaction timed out before completion.");
    }

    // Note that the verification may contain the side-effect of
    // cleaning up the edit state if it finds that the current edit state
    // relates to a defunct transaction.
    grpc::Status ret_val = VerifyNoConflictingEdits(tx, edit_time);
    if (!ret_val.ok()) {
      return ret_val;
    }
    edit_tx_ = tx;
    edit_value_ = e;
    edit_state_time_.state = SETTING;
    edit_state_time_.time = edit_time;
    return grpc::Status::OK;
  }

  grpc::Status Remove(Transaction* tx, uint64_t edit_time) {
    ExclusiveLock guard(guard_);
    Transaction::AtomicInfo current_tx_info = tx->GetAtomicInfo();
    if (current_tx_info.operation_start_time != edit_time) {
      return grpc::Status(grpc::StatusCode::DEADLINE_EXCEEDED,
        "The transaction timed out before completion.");
    }
    // Note that the verification may contain the side-effect of
    // cleaning up the edit state if it finds that the current edit state
    // relates to a defunct transaction.
    grpc::Status ret_val = VerifyNoConflictingEdits(tx, edit_time);
    if (!ret_val.ok()) {
      return ret_val;
    }
    edit_tx_ = tx;
    edit_value_ = not_present_value_;
    edit_state_time_.state = REMOVING;
    edit_state_time_.time = edit_time;
  }

  void CompleteWrite(const Transaction* tx) {
    ExclusiveLock guard(guard_);
    CompleteWriteWithGuard(tx);
  }

  void Rollback(const Transaction* tx) {
    ExclusiveLock guard(guard_);
    if (tx != edit_tx_) {
      return;
    }
    ClearEditState();
  }

  // Removes versions if their time-span does not cover clean_time, and if
  // their time-span comes before clean_time. i.e.:
  // remove if version.times.remove <= clean_time.
  void CleanVersions(uint64_t clean_time) {
    ExclusiveLock guard(versions_guard_);
    uint16_t version_count = historical_versions_.size();
    if (versions_start_ > versions_next_) {
      while (versions_start_ < version_count &&
             historical_versions_[versions_start_].times.remove <= clean_time) {
        versions_start_++;
      }
      if (versions_start_ == version_count) {
        versions_start_ = 0;
      }
    }
    while (versions_next_ != versions_start_ &&
           historical_versions_[versions_start_].times.remove <= clean_time) {
      versions_start_++;
    }
  }

 private:
  // Assumes you already have guard_ locked.
  void CompleteWriteWithGuard(const Transaction* tx) {
    if (edit_tx_ != tx) {
      // This is perfectly valid: It might be that another transaction
      // that encountered this TxAware instance when tx was in COMPLETING_WRITE
      // state may have pushed the edits into completion already.
      return;
    }
    Transaction::AtomicInfo tx_info = tx->GetAtomicInfo();
    if (tx_info.state != Transaction::COMPLETING_WRITE ||
        tx_info.operation_start_time != edit_state_time_.time) {
      return;
    }

    if (current_value_times_.remove == acumio::time::END_OF_TIME) {
      ExclusiveLock versions_lock(versions_guard_);
      IncreaseVersionsBufferIfNeeded();
      Version& next_version = historical_versions_[versions_next_];
      next_version.value = current_value_;
      next_version.times.create = current_value_times_.create;
      next_version.times.remove = edit_state_time_.time;
      versions_next_++;
      if (versions_next_ == historical_versions_.size()) {
        versions_next_ = 0;
      }
    }
    // If current_value_times_.remove != END_OF_TIME, there is no reason
    // to push a value onto the history stack since the gap in time will
    // indicate a lack of value at that time.

    current_value_ = edit_value_;
    current_value_times_.create = edit_state_time_.time;
    current_value_times_.remove = (edit_state_time_.state == REMOVING ?
        edit_state_time_.time : acumio::time::END_OF_TIME);
  }

  // Assumes you have locked guard_.
  void ClearEditState() {
    edit_value_ = not_present_value_;
    edit_state_time_.state = NOT_EDITING;
    edit_state_time_.time = UINT64_C(0);
    edit_tx_ = nullptr;
  }

  // Assumes you have locked versions.
  uint16_t VersionCount() const {
    if (versions_start_ < versions_next_) {
      return versions_next_ - versions_start_;
    }
    return historical_versions_.size() + versions_next_ - versions_start_;
  }

  void IncreaseVersionsBufferIfNeeded() {
    uint16_t version_count_plus_one = VersionCount() + 1;
    if (version_count_plus_one < historical_versions_.size()) {
      return;
    }

    // We should add more buffer. Two cases: the simple
    // case where we just need to add empty contents to the end
    // and the more complex case where we need to migrate the
    // versions to make sure we get a valid ordering.
    if (versions_start_ > versions_next_) {
      // Here, we will copy the old contents to the end.
      // This is the more complex case. Sadly, after repeated migrations,
      // it's also the most common.
      uint16_t old_size = historical_versions_.size();
      for (uint16_t i = versions_start_; i < old_size; i++) {
        historical_versions_.push_back(historical_versions_[i]);
      }
      for (uint16_t i = 0; i < versions_next_; i++) {
        historical_versions_.push_back(historical_versions_[i]);
      }
      versions_start_ = old_size;
      versions_next_ = 0;
    }
    else {
      // This is the simple case.
      TimeBoundary empty_boundary(0, 0);
      Version empty_version(not_present_value_, empty_boundary);
      for (uint16_t i = 0; i < version_count_plus_one; i++) {
        historical_versions_.push_back(empty_version);
      }
    }
  }

  const EltType& VersionSearch(
      uint64_t access_time, bool* exists_at_time) const {
    // The access time pre-dates the current value create time.
    // Time to look at prior history.
    SharedLock read_lock(versions_guard_);
    if (versions_start_ == versions_next_) {
      *exists_at_time = false;
      return not_present_value_;
    }

    // We want to binary search the history queue, but we want to make
    // the logic for doing so simple. This is of course complicated by
    // the fact that we are using a circle queue. We are going to potentially
    // check 4 boundary values:
    //     1) The earliest value (at versions_start_),
    //     2) the latest value (at versions_next_ - 1 if
    //        versions_next_ != 0, otherwise, at
    //        historical_versions_.size() - 1),
    // And then, if versions_next_ != 0 and versions_next_ < versions_start_,
    //     3) The version at the last position in the vector
    //     4) The version at the 0th position in the vector.
    // By checking at the beginning and ending vector positions, our binary
    // search will require fewer edge cases.
    const Version& early_bound = historical_versions_[versions_start_];
    if (early_bound.times.create < access_time) {
      *exists_at_time = false;
      return not_present_value_;
    }
    if (early_bound.times.remove > access_time) {
      *exists_at_time = true;
      return early_bound.value;
    }
    // We have now established that the earliest version comes before
    // our access time. Next, we check our latest version.
    const Version& late_bound = historical_versions_[versions_next_];
    if (late_bound.times.remove >= access_time) {
      *exists_at_time = false;
      return not_present_value_;
    }
    if (late_bound.times.create >= access_time) {
      *exists_at_time = true;
      return late_bound.value;
    }
    uint16_t early_position = versions_start_;
    uint16_t late_position = versions_next_;
    // We have now established early and late bounds, but we need to
    // handle the case of spanning across the end of the versions vector.
    if (versions_next_ < versions_start_) {
      uint16_t end_pos = historical_versions_.size() - 1;
      const Version& vector_end = historical_versions_[end_pos];
      if (vector_end.times.create <= access_time) {
        if (access_time < vector_end.times.remove) {
          *exists_at_time = true;
          return vector_end.value;
        }
        //vector_end.times.remove <= access_time
        const Version& vector_start = historical_versions_[0];
        if (access_time < vector_start.times.remove) {
          // vector_end.times.remove <= access_time < vector_start.times.remove
          // so either we match on historical_versions_[0] or there is no
          // match.
          if (vector_start.times.create <= access_time) {
            *exists_at_time = true;
            return vector_start.value;
          }
          *exists_at_time = false;
          return not_present_value_;
        }
        // vector_start.times.remove <= access_time
        early_position = 0;
      }
      else {
        late_position = end_pos;
      }
    }

    // Finally, we binary search the versions between early_position
    // and late_version. We maintain at each point that:
    // historical_versions_[early_position].remove <= access_time
    // < historical_versions_[late_position].create
    while (late_position > early_position + 1) {
      uint16_t mid_position = (late_position + early_position)/2;
      const Version& mid_reference = historical_versions_[mid_position];
      if (mid_reference.times.create > access_time) {
        late_position = mid_position;
      }
      else if (access_time < mid_reference.times.remove){
        *exists_at_time = true;
        return mid_reference.value;
      }
      else {
        early_position = mid_position;
      }
    }
    *exists_at_time = false;
    return not_present_value_;
  }

  grpc::Status VerifyNoConflictingEdits(const Transaction* tx,
                                        uint64_t edit_time) {
    if (edit_tx_ != nullptr) {
      Transaction::AtomicInfo edit_tx_info = edit_tx_->GetAtomicInfo();
      switch(edit_tx_info.state) {
        case Transaction::NOT_STARTED: // Intentional fall-through to next case.
        case Transaction::READ:
          break;
        case Transaction::WRITE:
          if (edit_tx_->id() != tx->id()) {
            if (edit_tx_info.operation_start_time == edit_state_time_.time) {
              return grpc::Status(grpc::StatusCode::ABORTED,
                                  "concurrency exception.");
            }
            // If we reach here, the Transaction held in the edit information
            // has been timed out and repurposed. We simply clear the current
            // edit state (i.e., rolling back the change), and continue.
            ClearEditState();
          }
          // re-entering edit of same value with the same transaction.
          // While this might not be common behavior, it is perfectly valid in
          // terms of transactional consistency.
          break;
        case Transaction::COMPLETING_WRITE:
          CompleteWriteWithGuard(edit_tx_);
          break;
        case Transaction::COMMITTED: // Intentional fall-through to next case.
          // If we reach here, we have probably encountered the cruft of
          // a previously timed-out transaction where the transaction has
          // since then been re-purposed and then committed.
        case Transaction::ROLLED_BACK:
          ClearEditState();
          break;
        default: return grpc::Status(grpc::StatusCode::INTERNAL,
                                     "Coding issue. Did not fix switch.");
      }
    }
    if (edit_time < current_value_times_.create ||
        (current_value_times_.remove != acumio::time::END_OF_TIME &&
         edit_time < current_value_times_.remove)) {
      return grpc::Status(grpc::StatusCode::ABORTED, "concurrency exception.");
    }
    return grpc::Status::OK;
  }

  // If a process acquires both guard_ and versions_guard_, it must acquire
  // guard_ first.
  mutable SharedMutex guard_;  
  mutable SharedMutex versions_guard_;
  EltType current_value_;
  TimeBoundary current_value_times_;
  const EltType not_present_value_;
  EltType edit_value_;
  EditStateTime edit_state_time_;
  const Transaction* edit_tx_;
  uint16_t versions_start_;
  uint16_t versions_next_;
  std::vector<Version> historical_versions_;
};

} // namespace transaction
} // namespace acumio
#endif // AcumioServer_tx_aware_item_h
