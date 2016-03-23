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
  grpc::Status AssociateNamespace(const model::Namespace& found_namespace,
                                  const model::Description& found_description,
                                  const model::Description& update_description,
                                  const std::string& desired_separator);

  grpc::Status CreateAssociatedNamespace(const std::string& full_name,
                                         const std::string& parent_full_name,
                                         const std::string& name,
                                         const std::string& separator,
                                         const model::Description& desc);

  grpc::Status CreateOrAssociateNamespace(const model::Namespace& parent,
                                          const std::string& name,
                                          const std::string& separator,
                                          const model::Description& desc);

  // Used internally when we have a GetRepository with the request to just
  // get the Repository without the Descriptions.
  grpc::Status GetJustRepository(
      const model::QualifiedName& repository_name,
      model::server::GetRepositoryResponse* response);

  grpc::Status GetParentNamespace(const model::QualifiedName& repository_name,
                                  model::Namespace* parent) const;

  // Checks to see if provided Namespace has any elements in it:
  // either Namespaces, or Repositories (or later, TODO: Datasets).
  // TODO: Create a unified InternalNamespaceService class for providing
  // common code between NamespaceService and RepositoryService. This
  // is otherwise copying quite a bit of code.
  grpc::Status IsNamespaceEmpty(const model::Namespace& name_space,
                                bool* result) const;

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

  grpc::Status ValidateNewRepository(const model::Repository& repository) const;

  // Returns OK status if the Namespace with the same name as repository_name
  // exists, and is associated with a repository.
  grpc::Status VerifyAssociatedNamespaceExists(
      const model::Namespace& parent,
      const model::QualifiedName& repository_name) const;

  std::shared_ptr<RepositoryRepository> repository_;
  std::shared_ptr<NamespaceRepository> namespace_repository_;
};

} // namespace acumio
#endif // AcumioServer_RepositoryService_h
