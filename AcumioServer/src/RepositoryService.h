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
#include "namespace_repository.h"
#include "repository_repository.h"

namespace acumio {

class RepositoryService {
 public:
  RepositoryService(std::shared_ptr<RepositoryRepository> repository,
                    std::shared_ptr<NamespaceRepository> namespace_repository);
  ~RepositoryService();

  grpc::Status CreateRepository(const model::Repository& repository,
                                const model::Description& description);

  grpc::Status GetRepository(const model::QualifiedName& repository_name,
                             model::Repository_Type type,
                             bool include_description,
                             bool include_description_history,
                             model::server::GetRepositoryResponse* response);

  grpc::Status ListRepositories(
      uint32_t list_max,
      const model::QualifiedName& start_after_name,
      model::Repository_Type start_after_type,
      bool include_descriptions,
      model::server::ListRepositoriesResponse* response);

  grpc::Status RemoveRepository(const model::QualifiedName& repository_name,
                                model::Repository_Type type,
                                bool force);

  grpc::Status UpdateRepository(const model::QualifiedName& repository_name,
                                model::Repository_Type type,
                                const model::Repository& update);

  grpc::Status UpdateRepositoryWithDescription(
      const model::QualifiedName& repository_name,
      model::Repository_Type repository_type,
      const model::Repository& update,
      const model::Description& updated_description,
      bool clear_description);

  grpc::Status UpsertRepositoryDescription(
      const model::QualifiedName& repository_name,
      model::Repository_Type repository_type,
      const model::Description& update,
      bool clear_description);

 private:
  // Used internally when we have a GetRepository with the request to just
  // get the Repository without the Descriptions.
  grpc::Status GetJustRepository(
      const model::QualifiedName& repository_name,
      model::Repository_Type repository_type,                           
      model::server::GetRepositoryResponse* response);

  grpc::Status ValidateNewRepository(const model::Repository& repository);

  grpc::Status ValidateRepositoryRemoval(
      const model::QualifiedName& name,
      model::Repository_Type type);

  grpc::Status ValidateRepositoryUpdate(
      const model::QualifiedName& name,
      model::Repository_Type type,
      const model::Repository& repository);

  RepositoryRepository::PrimaryIterator IteratorStart(
    const model::QualifiedName& start_after_name,
    model::Repository_Type start_after_type) const;

  grpc::Status ListJustRepositories(
      RepositoryRepository::PrimaryIterator it,
      uint32_t list_max,
      model::server::ListRepositoriesResponse* response);

  grpc::Status ListRepositoriesAndDescriptions(
      RepositoryRepository::PrimaryIterator it,
      uint32_t list_max,
      model::server::ListRepositoriesResponse* response);

  std::shared_ptr<RepositoryRepository> repository_;
  std::shared_ptr<NamespaceRepository> namespace_repository_;
};

} // namespace acumio
#endif // AcumioServer_RepositoryService_h
