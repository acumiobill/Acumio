//============================================================================
// Name        : comparable.cc
// Author      : Bill Province (bill@acumio.com)
// Version     :
// Copyright   : Copyright 2016
// Description : A virtual class similar to Java's "Comparable" interface.
//               be of different types, some of which may be complex.
//============================================================================
#include "comparable.h"

namespace acumio {
Comparable::~Comparable() {}
StringComparable::~StringComparable() {}
StringPairComparable::~StringPairComparable() {}
Int32Comparable::~Int32Comparable() {}
Int64Comparable::~Int64Comparable() {}
} // namespace acumio
