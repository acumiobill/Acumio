#ifndef AcumioServer_repository_repository_h
#define AcumioServer_repository_repository_h
//============================================================================
// Name        : repository_repository.h
// Author      : Bill Province (bill@acumio.com)
// Version     :
// Copyright   : Copyright 2016
// Description : Provides multi-threaded access to an in-memory
//               repository of Repositories.
//               Note that we use the word "repository" here in two ways:
//               First, as part of an object model, where we model the notion
//               of a repository as a store-house of datasets. Second, where
//               we are storing our representation of the model, hence our
//               own internal "mem_repository" based repository.
//               When we use an adjective "repository" or "Repository" as
//               part of a name, we disambiguate by noting that these
//               are in different namespaces: there's model::Repository and
//               there's mem_repository::Repository. Finally, our notion
//               of a repository of repositories is called a
//               RepositoryRepository, and lives directly in the acumio
//               top-level namespace.
//============================================================================

#include <grpc++/support/status.h>
#include "described_repository.h"
#include "description.pb.h"
#include "repository.pb.h"

namespace acumio {

namespace model {

typedef Described<Repository> DescribedRepository;

} // namespace model

// We wrap an underlying _RepositoryRepository for a couple reasons:
// First, because we want to provide cleaner APIs than are available in the
// generic MemRepository, and second, because because an inheritance model
// can actually get in our way. Composition better-than Inheritance
// (Joshua Block: Effective Java rule number 14).
class RepositoryRepository {
 public:
  typedef mem_repository::DescribedRepository<model::Repository> _Repository;
  typedef _Repository::PrimaryIterator PrimaryIterator;
  typedef _Repository::SecondaryIterator SecondaryIterator;

  RepositoryRepository();
  ~RepositoryRepository();

  // TODO: Evaluate if we need both AddWithDescription and AddWithNoDescription.
  //       I suspect that we only need a single one, and it always has the
  //       description present.
  inline grpc::Status AddWithDescription(const model::Repository& repository,
                                         const model::Description& desc) {
    return repository_->AddWithDescription(repository, desc);
  }

  inline grpc::Status AddWithNoDescription(
      const model::Repository& repository) {
    return repository_->AddWithNoDescription(repository);
  }

  grpc::Status GetRepository(const model::QualifiedName& full_name,
                             model::Repository* elt) const;

  grpc::Status GetDescription(const model::QualifiedName& full_name,
                              model::Description* description) const;

  grpc::Status GetDescriptionHistory(
      const model::QualifiedName& full_name,
      model::DescriptionHistory* history) const;

  grpc::Status GetRepositoryAndDescription(
      const model::QualifiedName& full_name,
      model::Repository* repository,
      model::Description* description) const;

  grpc::Status GetRepositoryAndDescriptionHistory(
      const model::QualifiedName& full_name,
      model::Repository* repository,
      model::DescriptionHistory* history) const;

  grpc::Status Remove(const model::QualifiedName& full_name);

  grpc::Status UpdateNoDescription(const model::QualifiedName& full_name,
                                   const model::Repository& repository);

  grpc::Status ClearDescription(const model::QualifiedName& full_name);

  grpc::Status UpdateDescriptionOnly(
      const model::QualifiedName& full_name,
      const model::Description& description);

  grpc::Status UpdateAndClearDescription(
      const model::QualifiedName& full_name,
      const model::Repository& repository);

  grpc::Status UpdateWithDescription(
      const model::QualifiedName& full_name,
      const model::Repository& repository,
      const model::Description& description);

  PrimaryIterator LowerBoundByFullName(const model::QualifiedName& name) const;

  inline const PrimaryIterator primary_begin() const {
    return repository_->primary_begin();
  }

  inline const PrimaryIterator primary_end() const {
    return repository_->primary_end();
  }

  SecondaryIterator LowerBoundByNamespace(const std::string& name_space) const;

  inline const SecondaryIterator namespace_iter_begin() const {
    return repository_->secondary_begin(0);
  }

  inline const SecondaryIterator namespace_iter_end() const {
    return repository_->secondary_end(0);
  }

 private:
  std::unique_ptr<_Repository> repository_;
};

} // namespace acumio

#endif // AcumioServer_repository_repository_h
