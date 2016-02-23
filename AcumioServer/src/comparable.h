#ifndef AcumioServer_comparable_h
#define AcumioServer_comparable_h
//============================================================================
// Name        : comparable.h
// Author      : Bill Province (bill@acumio.com)
// Version     :
// Copyright   : Copyright 2016
// Description : A virtual class similar to Java's "Comparable" interface.
//
//               This class introduces some potentially annoying "features"
//               such as the apparent ability of the interface to compare two
//               "Comparable" objects that are not actually Comparable. For
//               example, comparing a std::string to an int.
//
//               However, if we want a collection of "things" each
//               specialized by potentially different comparables, then this
//               allows us to do so in the interface. Example: I want to
//               have a repository with multiple indices. The indices may
//               be of different types, some of which may be complex.
//============================================================================

#include <string>

namespace acumio {
class Comparable {
 public:
  Comparable() {}
  virtual ~Comparable();
  virtual int compare_to(const Comparable& c) const = 0;
  virtual std::string to_string() const = 0;
  inline bool operator==(const Comparable& c) const {
    return compare_to(c) == 0;
  }
  inline bool operator<(const Comparable& c) const {
    return compare_to(c) < 0;
  }
  inline bool operator>(const Comparable& c) const {
    return compare_to(c) > 0;
  }
  inline bool operator <=(const Comparable& c) const {
    return compare_to(c) <= 0;
  }
  inline bool operator >=(const Comparable& c) const {
    return compare_to(c) >= 0;
  }
  inline bool operator !=(const Comparable& c) const {
    return compare_to(c) != 0;
  }
};

class StringComparable : public Comparable {
 public:
  StringComparable(const std::string& value) : Comparable(), value_(value) {}
  ~StringComparable();
  inline const std::string& value() const { return value_; }
  inline int compare_to(const Comparable& c) const {
    return value_.compare((dynamic_cast<const StringComparable&>(c)).value());
  }
  inline std::string to_string() const { return value_; }
 private:
  std::string value_;
};

class Int32Comparable : public Comparable {
 public:
  Int32Comparable(int32_t value) : Comparable(), value_(value) {}
  ~Int32Comparable();
  inline int32_t value() const { return value_; }
  inline int compare_to(const Comparable& c) const {
    return value_ - (dynamic_cast<const Int32Comparable&>(c)).value();
  }
  inline std::string to_string() const { return std::to_string(value_); }
 private:
  int32_t value_;
};

class Int64Comparable : public Comparable {
 public:
  Int64Comparable(int64_t value) : Comparable(), value_(value) {}
  ~Int64Comparable();
  inline int64_t value() const { return value_; }
  inline int compare_to(const Comparable& c) const {
    return value_ - (dynamic_cast<const Int64Comparable&>(c)).value();
  }
  inline std::string to_string() const { return std::to_string(value_); }
 private:
  int64_t value_;
};

} // namespace acumio
#endif // AcumioServer_comparable_h
