#ifndef AcumioServer_namespace_repository_h
#define AcumioServer_namespace_repository_h
//============================================================================
// Name        : namespace_repository.h
// Author      : Bill Province (bill@acumio.com)
// Version     :
// Copyright   : Copyright (C) 2016 Acumio
// Description : Provides multi-threaded access to an in-memory
//               repository of Namespaces.
//============================================================================

#include <grpc++/support/status.h>
#include "described_repository.h"
#include "description.pb.h"
#include "names.pb.h"

namespace acumio {

namespace model {

typedef Described<Namespace> DescribedNamespace;

} // namespace model

// We wrap an underlying _NamespaceRepository for a couple reasons:
// First, because we want to provide cleaner APIs than are available in the
// generic MemRepository, and second, because because an inheritance model
// can actually get in our way. Composition better-than Inheritance
// (Joshua Block: Effective Java rule number 14).
class NamespaceRepository {
 public:
  typedef mem_repository::DescribedRepository<model::Namespace> _Repository;
  typedef _Repository::PrimaryIterator PrimaryIterator;
  typedef _Repository::SecondaryIterator SecondaryIterator;

  NamespaceRepository();
  ~NamespaceRepository();

  inline grpc::Status AddWithDescription(const model::Namespace& name_space,
                                         const model::Description& desc) {
    return repository_->AddWithDescription(name_space, desc);
  }

  inline grpc::Status AddWithNoDescription(const model::Namespace& name_space) {
    return repository_->AddWithNoDescription(name_space);
  }

  inline grpc::Status GetNamespace(const std::string& full_name,
                             model::Namespace* elt) const {
    std::unique_ptr<Comparable> key(new StringComparable(full_name));
    return repository_->GetEntity(key, elt);
  }

  inline grpc::Status GetDescription(const std::string& full_name,
                                     model::Description* description) const {
    std::unique_ptr<Comparable> key(new StringComparable(full_name));
    // TODO: Translate NOT_FOUND results to present better error message.
    //       Perhaps the best way is to push the information down to
    //       the constructor of the generic repository so that it can
    //       properly form the error message at the outset.
    return repository_->GetDescription(key, description);
  }

  inline grpc::Status GetDescriptionHistory(
      const std::string& full_name, model::DescriptionHistory* history) const {
    std::unique_ptr<Comparable> key(new StringComparable(full_name));
    // TODO: Translate NOT_FOUND results to present better error message.
    //       Perhaps the best way is to push the information down to
    //       the constructor of the generic repository so that it can
    //       properly form the error message at the outset.
    return repository_->GetDescriptionHistory(key, history);
  }

  inline grpc::Status GetNamespaceAndDescription(
      const std::string& full_name,
      model::Namespace* name_space,
      model::Description* description) const {
    std::unique_ptr<Comparable> key(new StringComparable(full_name));
    // TODO: Translate NOT_FOUND results to present better error message.
    //       Perhaps the best way is to push the information down to
    //       the constructor of the generic repository so that it can
    //       properly form the error message at the outset.
    return repository_->GetEntityAndDescription(key, name_space, description);
  }

  inline grpc::Status GetNamespaceAndDescriptionHistory(
      const std::string& full_name,
      model::Namespace* name_space,
      model::DescriptionHistory* history) const {
    std::unique_ptr<Comparable> key(new StringComparable(full_name));
    // TODO: Translate NOT_FOUND results to present better error message.
    //       Perhaps the best way is to push the information down to
    //       the constructor of the generic repository so that it can
    //       properly form the error message at the outset.
    return repository_->GetEntityAndDescriptionHistory(key,
                                                       name_space,
                                                       history);
  }

  inline grpc::Status Remove(const std::string& full_name) {
    std::unique_ptr<Comparable> key(new StringComparable(full_name));
    return repository_->Remove(key);
  }

  inline grpc::Status UpdateNoDescription(const std::string& full_name,
                                          const model::Namespace& name_space) {
    std::unique_ptr<Comparable> key(new StringComparable(full_name));
    return repository_->UpdateNoDescription(key, name_space);
  }

  inline grpc::Status ClearDescription(const std::string& full_name) {
    std::unique_ptr<Comparable> key(new StringComparable(full_name));
    return repository_->ClearDescription(key);
  }

  inline grpc::Status UpdateDescriptionOnly(
      const std::string& full_name,
      const model::Description& description) {
    std::unique_ptr<Comparable> key(new StringComparable(full_name));
    return repository_->UpdateDescriptionOnly(key, description);
  }

  inline grpc::Status UpdateAndClearDescription(
      const std::string& full_name,
      const model::Namespace& name_space) {
    std::unique_ptr<Comparable> key(new StringComparable(full_name));
    return repository_->UpdateAndClearDescription(key, name_space);
  }

  inline grpc::Status UpdateWithDescription(
      const std::string& full_name,
      const model::Namespace& update,
      const model::Description& description) {
    std::unique_ptr<Comparable> key(new StringComparable(full_name));
    return repository_->UpdateWithDescription(key, update, description);
  }

  PrimaryIterator LowerBoundByFullName(const std::string& full_name) {
    std::unique_ptr<Comparable> key(new StringComparable(full_name));
    return repository_->LowerBound(key);
  }

  const PrimaryIterator primary_end() const {
    return repository_->primary_end();
  }

  SecondaryIterator LowerBoundByShortName(const std::string& short_name) {
    std::unique_ptr<Comparable> string_key(new StringComparable(short_name));
    return repository_->LowerBoundByIndex(string_key, 0);
  }

  const SecondaryIterator short_name_begin() const {
    return repository_->secondary_begin(0);
  }

  const SecondaryIterator short_name_end() const {
    return repository_->secondary_end(0);
  }

 private:
  std::unique_ptr<_Repository> repository_;
};

} // namespace acumio

#endif // AcumioServer_namespace_repository_h
