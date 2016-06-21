//============================================================================
// Name        : test_tx_flat_map.cpp
// Author      : Bill Province (bill@acumio.com)
// Version     :
// Copyright   : Copyright (C) 2016 Acumio
// Description : A test_driver for our TxAwareFlatMap class.
//============================================================================
#include "tx_aware_flat_map.h"

#include <gtest/gtest.h>
// #include <iostream> // commented out except when debuggin with std::cout.

#include "flat_map.h"
#include "gtest_extensions.h"
#include "object_allocator.h"
#include "string_allocator.h"
#include "transaction.h"

namespace acumio {
namespace {

const uint64_t one_second = acumio::time::NANOS_PER_SECOND;
// const uint64_t one_millisecond = acumio::time::NANOS_PER_MILLI;
// const uint64_t one_microsecond = acumio::time::NANOS_PER_MICRO;

using acumio::collection::FlatMap;
using acumio::collection::ObjectAllocator;
using acumio::collection::StringAllocator;
using acumio::collection::TxAwareFlatMap;

uint64_t NowTime() { return acumio::time::TimerNanosSinceEpoch(); }

TEST(FlatMapTest, Construction) {
  FlatMap<uint64_t> broken;
  StringAllocator key_allocator(8192);
  // More typical usage for object allocator would be to use a value-object
  // such as a model::Repository object.
  ObjectAllocator<uint64_t> object_allocator;
  FlatMap<uint64_t> map(&key_allocator, &object_allocator, 16, false);
}

TEST(TxAwareFlatMapTest, Construction) {
  TxAwareFlatMap<uint64_t> broken;
  ObjectAllocator<uint64_t> object_allocator;
  {
    // Putting in inner-scope to properly test destruction inside of
    // the test harness.
    // Use the provided object_allocator, use 8192 bytes max for string
    // allocation, and 16 entries per version with a 1-second cleanup
    // timeout.
    TxAwareFlatMap<uint64_t> map(&object_allocator, 8192, 16, one_second,
                                 NowTime());
  }
}

} // anonymous namespace
} // namespace acumio

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
