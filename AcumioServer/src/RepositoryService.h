#ifndef AcumioServer_RepositoryService_h
#define AcumioServer_RepositoryService_h
//============================================================================
// Name        : RepositoryService.h
// Author      : Bill Province (bill@acumio.com)
// Version     :
// Copyright   : Copyright 2016
// Description : Service for Repository operations.
//============================================================================

#include <grpc++/grpc++.h>
#include <server.pb.h>
#include <repository.pb.h>
#include "referential_service.h"
#include "repository_repository.h"

namespace acumio {

class RepositoryService {
 public:
  // Caller "owns" the pointers.
  RepositoryService(RepositoryRepository* repository,
                    ReferentialService* referential_service);
  ~RepositoryService();

  grpc::Status CreateRepository(const model::Repository& repository,
                                const model::Description& description,
                                bool create_or_associate_namespace,
                                const std::string& namespace_separator);

  grpc::Status GetRepository(const model::QualifiedName& repository_name,
                             bool include_description,
                             bool include_description_history,
                             model::server::GetRepositoryResponse* response);

  grpc::Status ListRepositories(
      uint32_t list_max,
      const model::QualifiedName& start_after_name,
      bool include_descriptions,
      model::server::ListRepositoriesResponse* response);

  grpc::Status RemoveRepository(const model::QualifiedName& repository_name,
                                bool force,
                                bool remove_or_disassociate_namespace);

  grpc::Status UpdateRepository(const model::QualifiedName& repository_name,
                                const model::Repository& update,
                                bool force);

  grpc::Status UpdateRepositoryWithDescription(
      const model::QualifiedName& repository_name,
      const model::Repository& update,
      const model::Description& updated_description,
      bool clear_description,
      bool force);

  grpc::Status UpsertRepositoryDescription(
      const model::QualifiedName& repository_name,
      const model::Description& update,
      bool clear_description);

 private:
  // Used internally when we have a GetRepository with the request to just
  // get the Repository without the Descriptions.
  grpc::Status GetJustRepository(
      const model::QualifiedName& repository_name,
      model::server::GetRepositoryResponse* response);

  // Returned Iterator guaranteed to be first Repository r such that
  // start_after_name < r.name(). Similar to the idea of LowerBound,
  // but LowerBound returns first element r such that after_name <= r.name().
  // This is used in conjuntion with the List API.
  RepositoryRepository::PrimaryIterator IteratorStart(
    const model::QualifiedName& start_after_name) const;

  grpc::Status ListJustRepositories(
      RepositoryRepository::PrimaryIterator it,
      uint32_t list_max,
      model::server::ListRepositoriesResponse* response) const;

  grpc::Status ListRepositoriesAndDescriptions(
      RepositoryRepository::PrimaryIterator it,
      uint32_t list_max,
      model::server::ListRepositoriesResponse* response) const;

  grpc::Status RemoveRepositoryWithForce(
      const model::QualifiedName& repository_name,
      model::Namespace& name_space,
      bool remove_or_disassociate_namespace);

  grpc::Status ValidateWellFormedRepository(
      const model::Repository& repository) const;

  // Returns OK status if the Namespace with the same name as repository_name
  // exists, and is associated with a repository.
  grpc::Status VerifyAssociatedNamespaceExists(
      const model::Namespace& parent,
      const model::QualifiedName& repository_name) const;

  RepositoryRepository* repository_;
  ReferentialService* referential_service_;
};

} // namespace acumio
#endif // AcumioServer_RepositoryService_h
