//============================================================================
// Name        : namespace_repository.cc
// Author      : Bill Province (bill@acumio.com)
// Version     :
// Copyright   : Copyright 2016
// Description : Provides multi-threaded access to an in-memory
//               repository of Namespaces.
//============================================================================

#include "namespace_repository.h"

namespace acumio {

namespace {
typedef mem_repository::KeyExtractorInterface<model::DescribedNamespace>
    _NamespaceExtractor;

class NamespaceKeyExtractor : public _NamespaceExtractor {
 public:
  NamespaceKeyExtractor() : _NamespaceExtractor() {}
  ~NamespaceKeyExtractor() {}
  inline std::unique_ptr<Comparable> GetKey(
      const model::DescribedNamespace& elt) const {
    std::unique_ptr<Comparable> key(
        new StringComparable(elt.name_space.full_name()));
    return key;
  }
};

class NamespaceShortKeyExtractor : public _NamespaceExtractor {
 public:
  NamespaceShortKeyExtractor() : _NamespaceExtractor() {}
  ~NamespaceShortKeyExtractor() {}
  inline std::unique_ptr<Comparable> GetKey(
    const model::DescribedNamespace& elt) const {
    std::unique_ptr<Comparable> key(
        new StringComparable(elt.name_space.name().name()));
    return key;
  }
};

} // anonymous namespace

NamespaceRepository::NamespaceRepository() : repository_(nullptr) {
  std::unique_ptr<_NamespaceExtractor> main_extractor(
      new NamespaceKeyExtractor());
  std::unique_ptr<_NamespaceExtractor> short_name_extractor(
      new NamespaceShortKeyExtractor());

  std::vector<std::unique_ptr<_NamespaceExtractor>> additional_extractors;
  additional_extractors.push_back(std::move(short_name_extractor));
  repository_.reset(new _Repository(std::move(main_extractor),
                                    &additional_extractors));
}

NamespaceRepository::~NamespaceRepository() {}

grpc::Status NamespaceRepository::Get(const std::string& full_name,
                                      model::Namespace* elt) const {
  std::unique_ptr<Comparable> key(new StringComparable(full_name));
  _Repository::StatusEltConstPtrPair getResult =
      repository_->NonMutableGet(key);
  if (!getResult.first.ok()) {
    return getResult.first;
  }
  elt->CopyFrom((getResult.second)->name_space);
  return grpc::Status::OK;
}

grpc::Status NamespaceRepository::Update(const std::string& full_name,
                                         const model::Namespace& name_space) {
  std::unique_ptr<Comparable> key(new StringComparable(full_name));
  _Repository::StatusEltPtrPair getResult = repository_->MutableGet(key);
  if (!getResult.first.ok()) {
    return getResult.first;
  }
  (getResult.second)->name_space.CopyFrom(name_space);
  return grpc::Status::OK;
}

} // namespace acumio
