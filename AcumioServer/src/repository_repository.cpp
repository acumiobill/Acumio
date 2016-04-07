//============================================================================
// Name        : repository_repository.cc
// Author      : Bill Province (bill@acumio.com)
// Version     :
// Copyright   : Copyright 2016
// Description : Provides multi-threaded access to an in-memory
//               repository of Repositories.
//============================================================================

#include "repository_repository.h"
#include <sstream>

#include "comparable.h"
#include "model_constants.h"

namespace acumio {

namespace {

typedef mem_repository::KeyExtractorInterface<model::Repository>
    _RepositoryExtractor;

class RepositoryKeyExtractor : public _RepositoryExtractor {
 public:
  RepositoryKeyExtractor() : _RepositoryExtractor() {}
  ~RepositoryKeyExtractor() {}
  inline std::unique_ptr<Comparable> GetKey(
      const model::Repository& repository) const {
    std::unique_ptr<Comparable> key(
        new StringPairComparable(repository.name().name_space(),
                                 repository.name().name()));
    return key;
  }
};

class RepositoryNamespaceExtractor : public _RepositoryExtractor {
 public:
  RepositoryNamespaceExtractor() : _RepositoryExtractor() {}
  ~RepositoryNamespaceExtractor() {}
  inline std::unique_ptr<Comparable> GetKey(
      const model::Repository& repository) const {
    std::unique_ptr<Comparable> key(
        new StringComparable(repository.name().name_space()));
    return key;
  }
};

} // anonymous namespace

RepositoryRepository::RepositoryRepository() : repository_(nullptr) {
  std::unique_ptr<_RepositoryExtractor> main_extractor(
      new RepositoryKeyExtractor());
  std::unique_ptr<_RepositoryExtractor> namespace_extractor(
      new RepositoryNamespaceExtractor());

  std::vector<std::unique_ptr<_RepositoryExtractor>> additional_extractors;
  additional_extractors.push_back(std::move(namespace_extractor));
  repository_.reset(new _Repository(std::move(main_extractor),
                                    &additional_extractors));
}

RepositoryRepository::~RepositoryRepository() {}

grpc::Status RepositoryRepository::GetRepository(
    const model::QualifiedName& full_name,
    model::Repository* elt) const {
  std::unique_ptr<Comparable> key(
      new StringPairComparable(full_name.name_space(),
                               full_name.name()));
  return repository_->GetEntity(key, elt);
}

grpc::Status RepositoryRepository::GetDescription(
    const model::QualifiedName& full_name,
    model::Description* description) const {
  std::unique_ptr<Comparable> key(
      new StringPairComparable(full_name.name_space(),
                               full_name.name()));
  // TODO: Translate NOT_FOUND results to present better error message.
  // See similar comment in NamespaceRepository::GetDescription.
  return repository_->GetDescription(key, description);
}

grpc::Status RepositoryRepository::GetDescriptionHistory(
    const model::QualifiedName& full_name,
    model::DescriptionHistory* history) const {
  std::unique_ptr<Comparable> key(
      new StringPairComparable(full_name.name_space(), full_name.name()));
  // TODO: Translate NOT_FOUND results to present better error message.
  // See similar comment in NamespaceRepository::GetDescription.
  return repository_->GetDescriptionHistory(key, history);
}

grpc::Status RepositoryRepository::GetRepositoryAndDescription(
    const model::QualifiedName& full_name,
    model::Repository* repository,
    model::Description* description) const {
  std::unique_ptr<Comparable> key(
      new StringPairComparable(full_name.name_space(), full_name.name()));
  return repository_->GetEntityAndDescription(key, repository, description);
}

grpc::Status RepositoryRepository::GetRepositoryAndDescriptionHistory(
    const model::QualifiedName& full_name,
    model::Repository* repository,
    model::DescriptionHistory* history) const {
  std::unique_ptr<Comparable> key(
      new StringPairComparable(full_name.name_space(), full_name.name()));
  return repository_->GetEntityAndDescriptionHistory(key, repository, history);
}

grpc::Status RepositoryRepository::Remove(
    const model::QualifiedName& full_name) {
  std::unique_ptr<Comparable> key(
      new StringPairComparable(full_name.name_space(), full_name.name()));
  return repository_->Remove(key);
}

grpc::Status RepositoryRepository::UpdateNoDescription(
    const model::QualifiedName& full_name,
    const model::Repository& repository) {
  std::unique_ptr<Comparable> key(
      new StringPairComparable(full_name.name_space(), full_name.name()));
  return repository_->UpdateNoDescription(key, repository);
}

grpc::Status RepositoryRepository::ClearDescription(
    const model::QualifiedName& full_name) {
  std::unique_ptr<Comparable> key(
      new StringPairComparable(full_name.name_space(), full_name.name()));
  return repository_->ClearDescription(key);
}

grpc::Status RepositoryRepository::UpdateDescriptionOnly(
    const model::QualifiedName& full_name,
    const model::Description& description) {
  std::unique_ptr<Comparable> key(
      new StringPairComparable(full_name.name_space(), full_name.name()));
  return repository_->UpdateDescriptionOnly(key, description);
}

grpc::Status RepositoryRepository::UpdateAndClearDescription(
    const model::QualifiedName& full_name,
    const model::Repository& repository) {
  std::unique_ptr<Comparable> key(
      new StringPairComparable(full_name.name_space(), full_name.name()));
  return repository_->UpdateAndClearDescription(key, repository);
}

grpc::Status RepositoryRepository::UpdateWithDescription(
    const model::QualifiedName& full_name,
    const model::Repository& repository,
    const model::Description& description) {
  std::unique_ptr<Comparable> key(
      new StringPairComparable(full_name.name_space(), full_name.name()));
  return repository_->UpdateWithDescription(key, repository, description);
}

RepositoryRepository::PrimaryIterator
RepositoryRepository::LowerBoundByFullName(
    const model::QualifiedName& name) const {
  std::unique_ptr<Comparable> key(
      new StringPairComparable(name.name_space(), name.name()));
  return repository_->LowerBound(key);
}

RepositoryRepository::SecondaryIterator
RepositoryRepository::LowerBoundByNamespace(
    const std::string& name_space) const {
  std::unique_ptr<Comparable> key(new StringComparable(name_space));
  return repository_->LowerBoundByIndex(key, 0);
}

} // namespace acumio


