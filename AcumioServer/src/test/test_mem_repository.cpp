//============================================================================
// Name        : test_mem_repository.cpp
// Author      : Bill Province (bill@acumio.com)
// Version     :
// Copyright   : Copyright (C) 2016 Acumio
// Description : A test_driver for the MemRepository template class.
//               We have not yet pushed this test set to the limit,
//               because we indirectly test the MemRepository via the
//               various specialized Repository classes.
//               TODO: We should either bring this to completion or just
//               delete it. We will make that evaluation after we complete
//               the test cases for the other repositories.
//============================================================================
#include "mem_repository.h"
#include <iostream>
#include "comparable.h"
#include "test_macros.h"

namespace acumio {
namespace {
class MyClass {
 public:
  MyClass(std::string key, int32_t value) : key_(key), value_(value) {}
  ~MyClass() {}
  inline const std::string& key() const { return key_; }
  inline int32_t value() const { return value_; }

 private:
  std::string key_;
  int32_t value_;
};

typedef mem_repository::KeyExtractorInterface<MyClass> _MyClassExtractor;
class MyClassKeyExtractor : public _MyClassExtractor {
 public:
  MyClassKeyExtractor() : _MyClassExtractor() {}
  ~MyClassKeyExtractor() {}
  inline std::unique_ptr<Comparable> GetKey(const MyClass& element) const {
    std::unique_ptr<Comparable> key(new StringComparable(element.key()));
    return key;
  }
};

class MyClassValueExtractor : public _MyClassExtractor {
 public:
  MyClassValueExtractor() : _MyClassExtractor() {}
  ~MyClassValueExtractor() {}
  
  inline std::unique_ptr<Comparable> GetKey(const MyClass& element) const {
    std::unique_ptr<Comparable> key(new Int32Comparable(element.value()));
    return key;
  }
};

TEST(MemRepository, RepositoryConstruction) {
  std::unique_ptr<_MyClassExtractor> key_extractor(new MyClassKeyExtractor());
  std::vector<std::unique_ptr<_MyClassExtractor>> empty_added_extractors;
  mem_repository::MemRepository<MyClass> basic_repository(
      std::move(key_extractor), &empty_added_extractors);

  // key_extractor moved the prior contents into basic_repository.
  // Need to reset it with a new extractor.
  key_extractor.reset(new MyClassKeyExtractor());
  std::unique_ptr<_MyClassExtractor> value_extractor(
      new MyClassValueExtractor());
  std::vector<std::unique_ptr<_MyClassExtractor>> added_extractors;
  added_extractors.push_back(std::move(value_extractor));
  mem_repository::MemRepository<MyClass> repository(
      std::move(key_extractor), &added_extractors);
}

TEST(MemRepository, AddElementToBasicRepository) {
  std::unique_ptr<_MyClassExtractor> key_extractor(new MyClassKeyExtractor());
  std::vector<std::unique_ptr<_MyClassExtractor>> empty_added_extractors;
  mem_repository::MemRepository<MyClass> basic_repository(
      std::move(key_extractor), &empty_added_extractors);

  MyClass elt("foo", 42);
  EXPECT_OK(basic_repository.Add(elt));
}

TEST(MemRepository, AddElementToMultiKeyedRepository) {
  std::unique_ptr<_MyClassExtractor> key_extractor(new MyClassKeyExtractor());
  std::unique_ptr<_MyClassExtractor> value_extractor(
      new MyClassValueExtractor());
  std::vector<std::unique_ptr<_MyClassExtractor>> added_extractors;
  added_extractors.push_back(std::move(value_extractor));
  mem_repository::MemRepository<MyClass> repository(std::move(key_extractor),
                                                    &added_extractors);

  MyClass elt("foo", 42);
  EXPECT_OK(repository.Add(elt));
}

TEST(MemRepository, AddMultipleElements) {
  std::unique_ptr<_MyClassExtractor> key_extractor(new MyClassKeyExtractor());
  std::unique_ptr<_MyClassExtractor> value_extractor(
      new MyClassValueExtractor());
  std::vector<std::unique_ptr<_MyClassExtractor>> added_extractors;
  added_extractors.push_back(std::move(value_extractor));
  mem_repository::MemRepository<MyClass> repository(std::move(key_extractor),
                                                    &added_extractors);

  MyClass elt("foo", 42);
  EXPECT_OK(repository.Add(elt));
  MyClass elt2("bar", 10);
  EXPECT_OK(repository.Add(elt2));
  MyClass elt3("zebra", 100);
  EXPECT_OK(repository.Add(elt3));
}

} // anonymous namespace
} // namespace acumio

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
