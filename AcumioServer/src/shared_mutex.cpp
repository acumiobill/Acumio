//============================================================================
// Name        : shared_mutex.cpp
// Author      : Bill Province (bill@acumio.com)
// Version     :
// Copyright   : Copyright (C) 2016 Acumio
// Description : SharedMutex implementation along with SharedLock and
//               ExlcusiveLock.
//============================================================================
#include "shared_mutex.h"

namespace acumio {
namespace transaction {

SharedMutex::SharedMutex() : state_(0) {}
SharedMutex::~SharedMutex() {}

void SharedMutex::break_locks() {
  state_.store(0);
}

void SharedMutex::acquire() {
  bool acquired = false;
  int16_t current_state = state_.load();
  while (!acquired) {
    while (current_state < 0) {
      current_state = state_.load();
    }
    int16_t next_state = current_state + 1;
    // If exchange fails, current_state is updated to reflect newest value.
    acquired = state_.compare_exchange_weak(current_state, next_state);
  }
}

void SharedMutex::release() {
  bool released = false;
  int16_t current_state = state_.load();
  // We compare current_state to 0 to consider the case where the locks
  // were explicitly broken.
  while (!released && current_state != 0) {
    int16_t next_state = current_state - 1;
    if (current_state < 0) {
      next_state += 2;
    }
    // If exchange fails, current_state is updated to reflect newest value.
    released = state_.compare_exchange_weak(current_state, next_state);
  }
}

void SharedMutex::acquire_exclusive() {
  bool enqueued = false;
  int16_t current_state = state_.load();
  while (!enqueued) {
    while (current_state < 0) {
      current_state = state_.load();
     }
    int16_t next_val = -(current_state + 1);
    // If exchange fails, current_state is updated to reflect newest value.
    enqueued = state_.compare_exchange_weak(current_state, next_val);
  }
  int16_t is_neg_one = state_.load();
  while (is_neg_one < -1) {
    is_neg_one = state_.load();
  }
}

void SharedMutex::release_exclusive() {
  state_.store(0);
}

SharedLock::SharedLock(SharedMutex& mutex) : mutex_(mutex) {
  mutex_.acquire();
}

SharedLock::~SharedLock() {
  mutex_.release();
}

ExclusiveLock::ExclusiveLock(SharedMutex& mutex) : mutex_(mutex) {
  mutex_.acquire_exclusive();
}

ExclusiveLock::~ExclusiveLock() {
  mutex_.release_exclusive();
}

} // namespace transaction
} // namespace acumio
