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

class NameNameTypeComparable : public Comparable {
 public:
  NameNameTypeComparable(const std::string& name_prefix,
                         const std::string& name_suffix,
                         model::Repository_Type type) :
      Comparable(), name_prefix_(name_prefix),
      name_suffix_(name_suffix), type_(type) {}
  ~NameNameTypeComparable() {}
  inline const std::string& name_prefix() const { return name_prefix_; }
  inline const std::string& name_suffix() const { return name_suffix_; }
  inline model::Repository_Type type() const { return type_; }
  int compare_to(const Comparable& c) const {
    const NameNameTypeComparable& other =
        dynamic_cast<const NameNameTypeComparable&>(c);
    if (type_ == other.type()) {
      int result = name_prefix_.compare(other.name_prefix());
      if (0 == result) {
        result = name_suffix_.compare(other.name_suffix());
      }
      return result;
    }
    int32_t left_type = (int32_t) type_;
    int32_t right_type = (int32_t) other.type();
    return (left_type > right_type ? 1 : -1);
  }

  inline std::string to_string() const {
    std::stringstream result;
    result << model::Repository_Type_Name(type_)
           << model::DEFAULT_NAMESPACE_SEPARATOR
           << name_prefix_
           << model::DEFAULT_NAMESPACE_SEPARATOR
           << name_suffix_;
    return result.str();
  }

 private:
  std::string name_prefix_;
  std::string name_suffix_;
  model::Repository_Type type_; 
};

typedef mem_repository::KeyExtractorInterface<model::Repository>
    _RepositoryExtractor;

class RepositoryKeyExtractor : public _RepositoryExtractor {
 public:
  RepositoryKeyExtractor() : _RepositoryExtractor() {}
  ~RepositoryKeyExtractor() {}
  inline std::unique_ptr<Comparable> GetKey(
      const model::Repository& repository) const {
    std::unique_ptr<Comparable> key(
        new NameNameTypeComparable(repository.name().name_space(),
                                   repository.name().name(),
                                   repository.type()));
    return key;
  }
};

class RepositoryNameExtractor : public _RepositoryExtractor {
 public:
  RepositoryNameExtractor() : _RepositoryExtractor() {}
  ~RepositoryNameExtractor() {}
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
  std::unique_ptr<_RepositoryExtractor> name_extractor(
      new RepositoryNameExtractor());
  std::unique_ptr<_RepositoryExtractor> namespace_extractor(
      new RepositoryNamespaceExtractor());

  std::vector<std::unique_ptr<_RepositoryExtractor>> additional_extractors;
  additional_extractors.push_back(std::move(name_extractor));
  additional_extractors.push_back(std::move(namespace_extractor));
  repository_.reset(new _Repository(std::move(main_extractor),
                                    &additional_extractors));
}

RepositoryRepository::~RepositoryRepository() {}

grpc::Status RepositoryRepository::GetRepository(
    const model::QualifiedName& full_name,
    model::Repository_Type type,
    model::Repository* elt) const {
  std::unique_ptr<Comparable> key(
      new NameNameTypeComparable(full_name.name_space(),
                                 full_name.name(),
                                 type));
  return repository_->GetEntity(key, elt);
}

grpc::Status RepositoryRepository::GetDescription(
    const model::QualifiedName& full_name,
    model::Repository_Type type,
    model::Description* description) const {
  std::unique_ptr<Comparable> key(
      new NameNameTypeComparable(full_name.name_space(),
                                 full_name.name(),
                                 type));
  // TODO: Translate NOT_FOUND results to present better error message.
  // See similar comment in NamespaceRepository::GetDescription.
  return repository_->GetDescription(key, description);
}

grpc::Status RepositoryRepository::GetDescriptionHistory(
    const model::QualifiedName& full_name,
    model::Repository_Type type,
    model::DescriptionHistory* history) const {
  std::unique_ptr<Comparable> key(
      new NameNameTypeComparable(full_name.name_space(),
                                 full_name.name(),
                                 type));
  // TODO: Translate NOT_FOUND results to present better error message.
  // See similar comment in NamespaceRepository::GetDescription.
  return repository_->GetDescriptionHistory(key, history);
}

grpc::Status RepositoryRepository::GetRepositoryAndDescription(
    const model::QualifiedName& full_name,
    model::Repository_Type type,
    model::Repository* repository,
    model::Description* description) const {
  std::unique_ptr<Comparable> key(
      new NameNameTypeComparable(full_name.name_space(),
                                 full_name.name(),
                                 type));
  return repository_->GetEntityAndDescription(key, repository, description);
}

grpc::Status RepositoryRepository::GetRepositoryAndDescriptionHistory(
    const model::QualifiedName& full_name,
    model::Repository_Type type,
    model::Repository* repository,
    model::DescriptionHistory* history) const {
  std::unique_ptr<Comparable> key(
      new NameNameTypeComparable(full_name.name_space(),
                                 full_name.name(),
                                 type));
  return repository_->GetEntityAndDescriptionHistory(key, repository, history);
}

grpc::Status RepositoryRepository::Remove(
    const model::QualifiedName& full_name,
    model::Repository_Type type) {
  std::unique_ptr<Comparable> key(
      new NameNameTypeComparable(full_name.name_space(),
                                 full_name.name(),
                                 type));
  return repository_->Remove(key);
}

grpc::Status RepositoryRepository::UpdateNoDescription(
    const model::QualifiedName& full_name,
    model::Repository_Type type,
    const model::Repository& repository) {
  std::unique_ptr<Comparable> key(
      new NameNameTypeComparable(full_name.name_space(),
                                 full_name.name(),
                                 type));
  return repository_->UpdateNoDescription(key, repository);
}

grpc::Status RepositoryRepository::ClearDescription(
    const model::QualifiedName& full_name,
    model::Repository_Type type) {
  std::unique_ptr<Comparable> key(
      new NameNameTypeComparable(full_name.name_space(),
                                 full_name.name(),
                                 type));
  return repository_->ClearDescription(key);
}

grpc::Status RepositoryRepository::UpdateDescriptionOnly(
    const model::QualifiedName& full_name,
    model::Repository_Type type,
    const model::Description& description) {
  std::unique_ptr<Comparable> key(
      new NameNameTypeComparable(full_name.name_space(),
                                 full_name.name(),
                                 type));
  return repository_->UpdateDescriptionOnly(key, description);
}

grpc::Status RepositoryRepository::UpdateAndClearDescription(
    const model::QualifiedName& full_name,
    model::Repository_Type type,
    const model::Repository& repository) {
  std::unique_ptr<Comparable> key(
      new NameNameTypeComparable(full_name.name_space(),
                                 full_name.name(),
                                 type));
  return repository_->UpdateAndClearDescription(key, repository);
}

grpc::Status RepositoryRepository::UpdateWithDescription(
    const model::QualifiedName& full_name,
    model::Repository_Type type,
    const model::Repository& repository,
    const model::Description& description) {
  std::unique_ptr<Comparable> key(
      new NameNameTypeComparable(full_name.name_space(),
                                 full_name.name(),
                                 type));
  return repository_->UpdateWithDescription(key, repository, description);
}

RepositoryRepository::PrimaryIterator
RepositoryRepository::LowerBoundByNameAndType(
    const model::QualifiedName& name,
    model::Repository_Type type) const {
  std::unique_ptr<Comparable> key(
      new NameNameTypeComparable(name.name_space(), name.name(), type));
  return repository_->LowerBound(key);
}

RepositoryRepository::SecondaryIterator RepositoryRepository::LowerBoundByName(
    const model::QualifiedName& name) const {
  std::unique_ptr<Comparable> key(new StringPairComparable(name.name_space(),
                                                           name.name()));
  return repository_->LowerBoundByIndex(key, 0);
}

RepositoryRepository::SecondaryIterator
RepositoryRepository::LowerBoundByNamespace(
    const std::string& name_space) const {
  std::unique_ptr<Comparable> key(new StringComparable(name_space));
  return repository_->LowerBoundByIndex(key, 1);
}

} // namespace acumio


