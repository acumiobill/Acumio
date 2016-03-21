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
typedef mem_repository::KeyExtractorInterface<model::Namespace>
    _NamespaceExtractor;

class NamespaceKeyExtractor : public _NamespaceExtractor {
 public:
  NamespaceKeyExtractor() : _NamespaceExtractor() {}
  ~NamespaceKeyExtractor() {}
  inline std::unique_ptr<Comparable> GetKey(
      const model::Namespace& name_space) const {
    std::unique_ptr<Comparable> key(
        new StringComparable(name_space.full_name()));
    return key;
  }
};

class NamespaceShortKeyExtractor : public _NamespaceExtractor {
 public:
  NamespaceShortKeyExtractor() : _NamespaceExtractor() {}
  ~NamespaceShortKeyExtractor() {}
  inline std::unique_ptr<Comparable> GetKey(
    const model::Namespace& name_space) const {
    std::unique_ptr<Comparable> key(
        new StringComparable(name_space.name().name()));
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
} // namespace acumio
