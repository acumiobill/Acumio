//============================================================================
// Name        : comparable.cpp
// Author      : Bill Province (bill@acumio.com)
// Version     :
// Copyright   : Copyright (C) 2016 Acumio
// Description : A virtual class similar to Java's "Comparable" interface.
//               be of different types, some of which may be complex.
//============================================================================
#include "comparable.h"

namespace acumio {
Comparable::~Comparable() {}
StringComparable::~StringComparable() {}

StringPairComparable::StringPairComparable(const std::string& prefix,
                                           const std::string& suffix) :
    Comparable(), prefix_(prefix), suffix_(suffix) {
  // The string libraries do not make it clear if they require space
  // reservation for the null-terminator when calling the reserve function,
  // or if it does it automatically.
  compare_string_.reserve(2 + prefix.length() + suffix.length());
  compare_string_.append(prefix).append("\x01").append(suffix);
}

StringPairComparable::StringPairComparable(const StringPairComparable& c) :
    Comparable(), prefix_(c.prefix_),
    suffix_(c.suffix_), compare_string_(c.compare_string_) {}

StringPairComparable::~StringPairComparable() {}
Int32Comparable::~Int32Comparable() {}
Int64Comparable::~Int64Comparable() {}
} // namespace acumio
