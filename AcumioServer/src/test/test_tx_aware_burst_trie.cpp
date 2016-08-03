//============================================================================
// Name        : test_tx_aware_burst_trie.cpp
// Author      : Bill Province (bill@acumio.com)
// Version     :
// Copyright   : Copyright (C) 2016 Acumio
// Description : A test_driver for our TxAwareFlatMap class.
//============================================================================
#include "tx_aware_burst_trie.h"

#include <gtest/gtest.h>
// #include <iostream> // commented out except when debugging with std::cout.

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
using acumio::collection::TxAwareBurstTrie;

uint64_t NowTime() { return acumio::time::TimerNanosSinceEpoch(); }

TEST(TxAwareBurstTrieTest, Construction) {
  TxAwareBurstTrie<uint64_t> broken;
  ObjectAllocator<uint64_t> object_allocator;
  {
    // Putting in inner-scope to properly test destruction inside of
    // the test harness.
    TxAwareBurstTrie<uint64_t> trie();
  }
}

} // anonymous namespace
} // namespace acumio

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
