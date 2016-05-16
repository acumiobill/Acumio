//============================================================================
// Name        : time_util.cpp
// Author      : Bill Province (bill@acumio.com)
// Version     :
// Copyright   : Copyright (C) 2016 Acumio
// Description : Utility functions and/or classes for working with time.
//============================================================================

#include "time_util.h"

#include <thread>

namespace acumio {
namespace time {

const uint64_t NANOS_PER_SECOND = 1000000000;
const uint64_t NANOS_PER_MILLI  = 1000000;
const uint64_t NANOS_PER_MICRO  = 1000;
const uint64_t END_OF_TIME = UINT64_MAX;

namespace {
typedef std::chrono::duration<uint64_t, std::nano> Nanos;
}

void SetTimestampToNow(google::protobuf::Timestamp* ts) {
  WallTime now = std::chrono::system_clock::now();
  int64_t epoch_nanos = std::chrono::duration_cast<Nanos>(
      now.time_since_epoch()).count();
  int64_t epoch_seconds = epoch_nanos/NANOS_PER_SECOND;
  int32_t remaining_nanos = (int32_t) (epoch_nanos % NANOS_PER_SECOND);
  ts->set_seconds(epoch_seconds);
  ts->set_nanos(remaining_nanos);
}

// This implementation should be good at least until around 2232 - and that's
// assuming it internally uses a signed 64-bit representation. If internal
// representation is unsigned, the implementation will last roughly 500 years.
uint64_t TimerNanosSinceEpoch() {
  TimerTime now = _steady_clock::now();
  return std::chrono::duration_cast<Nanos>(now.time_since_epoch()).count();
}

uint64_t LatestTimeoutTime(uint64_t timeout_nanos) {
  return TimerNanosSinceEpoch() - timeout_nanos;
}

void SleepNanos(uint64_t sleep_nanos) {
  std::this_thread::sleep_for(std::chrono::nanoseconds(sleep_nanos));
}

} // namespace time
} // namespace acumio
