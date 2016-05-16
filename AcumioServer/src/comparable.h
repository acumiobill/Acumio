#ifndef AcumioServer_comparable_h
#define AcumioServer_comparable_h
//============================================================================
// Name        : comparable.h
// Author      : Bill Province (bill@acumio.com)
// Version     :
// Copyright   : Copyright (C) 2016 Acumio
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
//
//               This additionally requires that the comparable have a
//               string-based comparison order; i.e., the interface has
//               a compare_string() method, and the requirement is that
//               for Comparables a and b (of consistent types) that
//               a.compare_to(b) < 0 iff
//               a.compare_string().compare(b.compare_string()) < 0.
//               While this may limit our notion of "Comparable" the advantage
//               is that we can put our Comparables into a Trie data structure.
//============================================================================

#include <string>

namespace acumio {
class Comparable {
 public:
  Comparable() {}
  virtual ~Comparable();
  virtual int compare_to(const Comparable& c) const = 0;
  virtual std::string to_string() const = 0;
  virtual const std::string& compare_string() const = 0;
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
  StringComparable(const StringComparable& c) :
      Comparable(), value_(c.value_) {}
  ~StringComparable();
  inline int compare_to(const Comparable& c) const {
    return value_.compare(c.compare_string());
  }
  inline std::string to_string() const { return value_; }
  inline const std::string& compare_string() const { return value_; }
  inline const std::string& value() const { return value_; }
 private:
  std::string value_;
};

class StringPairComparable : public Comparable {
 public:
  StringPairComparable(const std::string& prefix, const std::string& suffix);
  StringPairComparable(const StringPairComparable& c);
  ~StringPairComparable();
  inline const std::string& prefix() const { return prefix_; }
  inline const std::string& suffix() const { return suffix_; }
  inline int compare_to(const Comparable& c) const {
    return compare_string_.compare(c.compare_string());
  }
  inline std::string to_string() const { return prefix_ + " " + suffix_; }
  inline const std::string& compare_string() const { return compare_string_; }

 private:
  std::string prefix_;
  std::string suffix_;
  std::string compare_string_;
};

class Int32Comparable : public Comparable {
 public:
  Int32Comparable(int32_t value) : Comparable(), value_(value),
      compare_string_(std::to_string(value)) {}
  Int32Comparable(const Int32Comparable& other) : Comparable(),
      value_(other.value_), compare_string_(other.compare_string_) {}
  ~Int32Comparable();
  inline int32_t value() const { return value_; }
  inline int compare_to(const Comparable& c) const {
    return value_ - (dynamic_cast<const Int32Comparable&>(c)).value();
  }
  inline std::string to_string() const { return compare_string_; }
  inline const std::string& compare_string() const { return compare_string_; }
 private:
  int32_t value_;
  std::string compare_string_;
};

class Int64Comparable : public Comparable {
 public:
  Int64Comparable(int64_t value) : Comparable(), value_(value),
      compare_string_(std::to_string(value)) {}
  Int64Comparable(const Int64Comparable& other) : Comparable(),
      value_(other.value_), compare_string_(other.compare_string_) {}
  ~Int64Comparable();
  inline int64_t value() const { return value_; }
  inline int compare_to(const Comparable& c) const {
    return value_ - (dynamic_cast<const Int64Comparable&>(c)).value();
  }
  inline std::string to_string() const { return compare_string_; }
  inline const std::string& compare_string() const { return compare_string_; }
 private:
  int64_t value_;
  std::string compare_string_;
};

} // namespace acumio
#endif // AcumioServer_comparable_h
