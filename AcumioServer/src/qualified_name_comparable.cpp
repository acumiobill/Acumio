//============================================================================
// Name        : qualified_name_comparable.cpp
// Author      : Bill Province (bill@acumio.com)
// Version     :
// Copyright   : Copyright (C) 2016 Acumio
// Description : An implementation of the Comparable virtual class for
//               comparing acumio::model::QualifiedName instances.
//============================================================================
#include "qualified_name_comparable.h"

#include <sstream>
#include "model_constants.h"

namespace acumio {

QualifiedNameComparable::~QualifiedNameComparable() {}
int QualifiedNameComparable::compare_to(const Comparable& c) const {
  const model::QualifiedName& other =
      (dynamic_cast<const QualifiedNameComparable&>(c)).value();
  int namespace_comp = value_.name_space().compare(other.name_space());
  if (namespace_comp == 0) {
    return value_.name().compare(other.name());
  }
  return namespace_comp;
}

std::string QualifiedNameComparable::to_string() const {
  std::stringstream result;
  result << value_.name_space() << separator_ << value_.name();
  return result.str();
}
} // namespace acumio
