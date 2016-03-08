#ifndef AcumioServer_qualified_name_comparable_h
#define AcumioServer_qualified_name_comparable_h
//============================================================================
// Name        : qualified_name_comparable.h
// Author      : Bill Province (bill@acumio.com)
// Version     :
// Copyright   : Copyright 2016
// Description : An implementation of the Comparable virtual class for
//               comparing acumio::model::QualifiedName instances.
//============================================================================

#include <string>
#include <names.pb.h>
#include "comparable.h"

namespace acumio {
class QualifiedNameComparable : public Comparable {
 public:
  QualifiedNameComparable(const model::QualifiedName& value,
                          const std::string& separator) :
      Comparable(), value_(value), separator_(separator) {}
  ~QualifiedNameComparable();
  inline const model::QualifiedName& value() const { return value_; }
  int compare_to(const Comparable& c) const;
  std::string to_string() const;

 private:
  model::QualifiedName value_;
  std::string separator_;
};

} // namesapce acumio

#endif // AcumioServer_qualified_name_comparable_h
