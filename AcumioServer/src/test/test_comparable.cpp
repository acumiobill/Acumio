//============================================================================
// Name        : test_comparable.cpp
// Author      : Bill Province (bill@acumio.com)
// Version     :
// Copyright   : Copyright (C) 2016 Acumio
// Description : A test_driver for comparable.cpp.
//               TODO: Add a test to verify for each Comparable derivative
//               that for each Comparable a and b,
//               a < b  iff a.compare_string() < b.compare_string().
//============================================================================
#include "comparable.h"
#include <gtest/gtest.h>

namespace acumio {
TEST(StringComparableTest, BasicCompare) {
  std::string a_string = "a";
  std::string b_string = "b";
  std::string empty_string = "";
  std::string another_a = "a";
  std::string longer_a = "aa";
  StringComparable a_comparable(a_string);
  StringComparable b_comparable(b_string);
  StringComparable empty_comparable(empty_string);
  StringComparable another_comparable(another_a);
  StringComparable longer_a_comparable(longer_a);
  EXPECT_LT(a_comparable.compare_to(b_comparable), 0);
  EXPECT_LT(empty_comparable.compare_to(b_comparable), 0);
  EXPECT_EQ(a_comparable.compare_to(another_comparable), 0);
  EXPECT_EQ(a_comparable.compare_to(a_comparable), 0);
  EXPECT_GT(b_comparable.compare_to(longer_a_comparable), 0);
  EXPECT_GT(longer_a_comparable.compare_to(a_comparable), 0);
  EXPECT_GT(a_comparable.compare_to(empty_comparable), 0);
  EXPECT_EQ(empty_comparable.compare_to(empty_comparable), 0);
}

TEST(StringPairComparableTest, BasicCompare) {
  std::string a = "a";
  std::string aa = "aa";
  std::string b = "b";
  std::string empty = "";
  StringPairComparable empty_pair(empty, empty);
  StringPairComparable a_pair(a, a);
  StringPairComparable a_b(a, b);
  StringPairComparable b_a(b, a);
  StringPairComparable aa_empty(aa, empty);
  StringPairComparable empty_aa(empty, aa);
  EXPECT_EQ(empty_pair.compare_to(empty_pair), 0);
  EXPECT_EQ(a_pair.compare_to(a_pair), 0);
  EXPECT_LT(a_b.compare_to(b_a), 0);
  EXPECT_LT(a_pair.compare_to(aa_empty), 0);
  EXPECT_GT(a_pair.compare_to(empty_aa), 0);
  EXPECT_LT(a_pair.compare_to(a_b), 0);
  EXPECT_LT(empty_pair.compare_to(empty_aa), 0);
}

TEST(StringPairComparableTest, CompareString) {
  std::string prefix = "prefix";
  std::string suffix = "suffix";
  StringPairComparable comparable(prefix, suffix);
  std::string compare_string = comparable.compare_string();
  EXPECT_EQ(compare_string.size(), prefix.size() + suffix.size() + 1);
  EXPECT_EQ(compare_string[prefix.size()], '\x01');
}
} // namespace acumio

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
