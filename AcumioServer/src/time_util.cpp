//============================================================================
// Name        : time_util.cpp
// Author      : Bill Province (bill@acumio.com)
// Version     :
// Copyright   : Copyright 2016
// Description : Utility functions and/or classes for working with time.
//============================================================================

#include "time_util.h"

namespace acumio {
namespace time {

int64_t NANOS_PER_SECOND = 1000000000;

namespace {
typedef std::chrono::duration<int64_t, std::nano> Nanos;
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

} // namespace time
} // namespace acumio
