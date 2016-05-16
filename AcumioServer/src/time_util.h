#ifndef AcumioServer_time_util_h
#define AcumioServer_time_util_h
//============================================================================
// Name        : time_util.h
// Author      : Bill Province (bill@acumio.com)
// Version     :
// Copyright   : Copyright (C) 2016 Acumio
// Description : Utility functions and/or classes for working with time.
//============================================================================

#include <chrono>
#include <google/protobuf/timestamp.pb.h>
#include <type_traits>

namespace acumio {
namespace time {
// Number of Nanoseconds in a second. 10^9.
extern const uint64_t NANOS_PER_SECOND;
extern const uint64_t NANOS_PER_MILLI;
extern const uint64_t NANOS_PER_MICRO;
extern const uint64_t END_OF_TIME;

typedef std::chrono::time_point<std::chrono::system_clock> WallTime;
using _steady_clock = typename std::conditional<
    std::chrono::high_resolution_clock::is_steady,
    std::chrono::high_resolution_clock,
    std::chrono::steady_clock>::type;

// Use this time_point clock for precise timing measurements - such as
// defining timeouts for short duration operations. The intent here is
// that this clock should always be a steady timer, but we will use
// the high resolution clock if it is steady.
typedef std::chrono::time_point<_steady_clock> TimerTime;

// Uses the WallTime clock to define a google protobuf Timestamp value.
// Useful in the domain when working on a scale of changes where mostly,
// we care about accuracy at most to the nearest minute or so, and usually,
// a single days' worth of accuracy is fine. The recorded time is at a
// higher precision, just in case the precision is needed, but bear in
// mind that the clock involved may be subject to manual adjustments,
// and therefore, might not be a steady clock.
void SetTimestampToNow(google::protobuf::Timestamp* ts);

// Returns Nanos since the epoch using a steady clock. The notion of
// "steady" simply means that time values increase monotonically, and
// that each tick represents the same period of time. However, drift -
// caused by slightly variant clock speed - can certainly occur.
// One should expect that actually converting the Nanos since epoch
// to a real time value might give a different result than the WallClock
// time.
uint64_t TimerNanosSinceEpoch();

// Returns the number of nanos since epoch minus the timout_nanos parameter.
// We use this to detect timeout conditions:
// if (t <= LatestTimeoutTime(timeout_nanos)) then we know that the operation
// associated with time t should be considered timed out.
// Equivalent to TimerNanosSinceEpoch() - timeout_nanos.
uint64_t LatestTimeoutTime(uint64_t timeout_nanos);

void SleepNanos(uint64_t nanos);

} // namespace time
} // namespace acumio
#endif // AcumioServer_time_util_h
