#ifndef AcumioServer_time_util_h
#define AcumioServer_time_util_h
//============================================================================
// Name        : time_util.h
// Author      : Bill Province (bill@acumio.com)
// Version     :
// Copyright   : Copyright 2016
// Description : Utility functions and/or classes for working with time.
//============================================================================

#include <chrono>
#include <google/protobuf/timestamp.pb.h>
namespace acumio {
namespace time {
// Number of Nanoseconds in a second. 10^9.
extern int64_t NANOS_PER_SECOND;
typedef std::chrono::time_point<std::chrono::system_clock> WallTime;

void SetTimestampToNow(google::protobuf::Timestamp* ts);
} // namespace time
} // namespace acumio
#endif // AcumioServer_time_util_h
