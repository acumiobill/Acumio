//============================================================================
// Name        : test_mem_repository.cpp
// Author      : Bill Province (bill@acumio.com)
// Version     :
// Copyright   : Copyright (C) 2016 Acumio
// Description : A test_driver for the MemRepository template class.
//               We have not yet pushed this test set to the limit,
//               because we indirectly test the MemRepository via the
//               various specialized Repository classes.
//
//               TODO: We should bring these tests to completion.
//               The wrapper repository classes (DatasetRepository,
//               UserRepository, et.al.) mostly will provide test coverage
//               for MemRepository, but we should have it directly tested
//               as well.
//============================================================================
#include "mem_repository.h"
#include <iostream>
#include "comparable.h"
#include "gtest_extensions.h"

namespace acumio {
namespace {
class MyClass {
 public:
  MyClass(std::string key, std::string secondary, int32_t value) :
      key_(key), secondary_(secondary), value_(value) {}
  ~MyClass() {}
  inline const std::string& key() const { return key_; }
  inline const std::string& secondary() const { return secondary_; }
  inline int32_t value() const { return value_; }

 private:
  std::string key_;
  std::string secondary_;
  int32_t value_;
};

typedef mem_repository::KeyExtractorInterface<MyClass> _MyClassExtractor;
typedef mem_repository::MemRepository<MyClass> _MyClassRepository;
typedef mem_repository::MemRepository<MyClass>::RepositoryMap _MyClassMap;
typedef mem_repository::MemRepository<MyClass>::RepositoryMultiMap
    _MyClassMultiMap;

class MyClassKeyExtractor : public _MyClassExtractor {
 public:
  MyClassKeyExtractor() : _MyClassExtractor() {}
  ~MyClassKeyExtractor() {}
  inline std::unique_ptr<Comparable> GetKey(const MyClass& element) const {
    std::unique_ptr<Comparable> key(new StringComparable(element.key()));
    return key;
  }
};

class MyClassSecondaryExtractor : public _MyClassExtractor {
 public:
  MyClassSecondaryExtractor() : _MyClassExtractor() {}
  ~MyClassSecondaryExtractor() {}

  inline std::unique_ptr<Comparable> GetKey(const MyClass& element) const {
    std::unique_ptr<Comparable> key(new StringComparable(element.secondary()));
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

std::unique_ptr<_MyClassRepository> NewRepository() {
  std::unique_ptr<_MyClassExtractor> key_extractor(new MyClassKeyExtractor());
  std::vector<std::unique_ptr<_MyClassExtractor>> added_extractors;
  std::unique_ptr<_MyClassExtractor> secondary_extractor(
      new MyClassSecondaryExtractor());
  std::unique_ptr<_MyClassExtractor> value_extractor(
      new MyClassValueExtractor());
  added_extractors.push_back(std::move(secondary_extractor));
  added_extractors.push_back(std::move(value_extractor));
  std::unique_ptr<_MyClassRepository> repository(
      new _MyClassRepository(std::move(key_extractor), &added_extractors));
  return repository;
}

std::unique_ptr<_MyClassRepository> NewBasicRepository() {
  std::unique_ptr<_MyClassExtractor> key_extractor(new MyClassKeyExtractor());
  std::vector<std::unique_ptr<_MyClassExtractor>> added_extractors;
  std::unique_ptr<_MyClassRepository> repository(
      new _MyClassRepository(std::move(key_extractor), &added_extractors));
  return repository;
}

TEST(MemRepository, RepositoryConstruction) {
  std::unique_ptr<_MyClassRepository> basic_repository = NewBasicRepository();
  std::unique_ptr<_MyClassRepository> repository = NewRepository();
}

TEST(MemRepository, AddElementToBasicRepository) {
  std::unique_ptr<_MyClassRepository> basic_repository = NewBasicRepository();

  MyClass elt("foo", "bar", 42);
  EXPECT_OK(basic_repository->Add(elt));
}

TEST(MemRepository, AddElementToMultiKeyedRepository) {
  std::unique_ptr<_MyClassRepository> repository = NewRepository();

  MyClass elt("foo", "bar", 42);
  EXPECT_OK(repository->Add(elt));
}

TEST(MemRepository, AddMultipleElements) {
  std::unique_ptr<_MyClassRepository> repository = NewRepository();

  MyClass elt("foo", "bar", 42);
  EXPECT_OK(repository->Add(elt));
  MyClass elt2("bar", "bar", 10);
  EXPECT_OK(repository->Add(elt2));
  MyClass elt3("zebra", "dog", 100);
  EXPECT_OK(repository->Add(elt3));
}

TEST(MemRepository, RepositoryMultiMapOperations) {
  _MyClassMultiMap map;
  MyClass elt("foo", "bar", 42);
  std::unique_ptr<_MyClassExtractor> secondary_extractor(
      new MyClassSecondaryExtractor());
  std::unique_ptr<Comparable> secondary_key = secondary_extractor->GetKey(elt);
  map.emplace(std::move(secondary_key), 17);
  secondary_key = secondary_extractor->GetKey(elt);
  std::pair<_MyClassMultiMap::iterator,
            _MyClassMultiMap::iterator> range = map.equal_range(secondary_key);
  _MyClassMultiMap::iterator lower = range.first;
  const std::unique_ptr<Comparable>& bar_key = lower->first;
  EXPECT_TRUE(bar_key);
  ASSERT_EQ(map.size(), 1);
  map.erase(lower);
  ASSERT_EQ(map.size(), 0);
}

TEST(MemRepository, AddEltThenRemove) {
  std::unique_ptr<_MyClassRepository> repository = NewRepository();
  MyClass elt("foo", "bar", 42);
  EXPECT_OK(repository->Add(elt));
  MyClassKeyExtractor extractor;
  MyClassSecondaryExtractor secondary_extractor;
  std::unique_ptr<Comparable> elt_key = extractor.GetKey(elt);
  std::unique_ptr<Comparable> bar_key = secondary_extractor.GetKey(elt);
  _MyClassRepository::SecondaryIterator iter =
      repository->LowerBoundByIndex(bar_key, 0);
  const std::unique_ptr<Comparable>& bar_found = iter->first;
  ASSERT_TRUE(bar_key);
  EXPECT_EQ(bar_found->to_string(), "bar");
  ASSERT_EQ(repository->size(), 1);
  EXPECT_OK(repository->Remove(elt_key));
  ASSERT_EQ(repository->size(), 0);
}

} // anonymous namespace
} // namespace acumio

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
