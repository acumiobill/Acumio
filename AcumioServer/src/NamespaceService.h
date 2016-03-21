#ifndef AcumioServer_NamespaceService_h
#define AcumioServer_NamespaceService_h
//============================================================================
// Name        : NamespaceService.h
// Author      : Bill Province (bill@acumio.com)
// Version     :
// Copyright   : Copyright 2016
// Description : Service for Namespace operations.
//============================================================================

#include <grpc++/grpc++.h>
#include <server.pb.h>
#include <names.pb.h>
#include "namespace_repository.h"
#include "repository_repository.h"

namespace acumio {

class NamespaceService {
 public:
  NamespaceService(std::shared_ptr<NamespaceRepository> repository,
                   std::shared_ptr<RepositoryRepository> repository_repository);
  ~NamespaceService();

  grpc::Status CreateNamespace(const model::Namespace& name_space,
                               const model::Description& description);

  grpc::Status GetNamespace(const std::string& full_namespace,
                            bool include_description,
                            bool include_description_history,
                            model::server::GetNamespaceResponse* response);

  grpc::Status RemoveNamespace(const std::string& namespace_name);

  grpc::Status UpdateNamespace(const std::string& namespace_name,
                               const model::Namespace& update);

  grpc::Status UpdateNamespaceWithDescription(
      const std::string& namespace_name,
      const model::Namespace& update,
      const model::Description& updated_description,
      bool clear_description);

  grpc::Status UpsertNamespaceDescription(const std::string& described,
      const model::Description& update, bool clear_description);

 private:
  // Used internally when we have a GetNamespace with the request to just
  // get the Namespace without the Descriptions.
  grpc::Status GetJustNamespace(const std::string& full_namespace,
                                model::server::GetNamespaceResponse* response);

  // Used to verify that a new Namespace does not violate any rules.
  // The rules-checking includes only those rules that are not already
  // enforced by the Repository layer - hence, it does not include primary
  // key violation checking.
  grpc::Status ValidateNewNamespace(const model::Namespace& name_space);

  grpc::Status ValidateNamespacePlusSeparatorRemoval(
      const model::Namespace& name_space);
  // Verifies that removing the Namespace with the given name would not
  // generate Orphan entities.
  // TODO: Currently, this only supports Namespace and Repository orphans.
  // Later, we need to add Datasets.
  grpc::Status ValidateNamespaceRemoval(const std::string& namespace_name);

  // Used to verify that an update to a Namespace does not violate any rules.
  // Basically, we validate that the update result is good for a new
  // Namespace in terms of being well-formed in addition to being a valid
  // Delete operation if the full_name of the Namespace is being modified
  // or if the separator is changing. (The reason that the separator change
  // causes us headaches is because the full-name of any enclosed entities
  // will include the old separator).
  grpc::Status ValidateNamespaceUpdate(const std::string& namespace_name,
                                       const model::Namespace& update);

  // This is a shared ptr instead of a unique ptr because other services may
  // also need access to the Namespace repository.
  std::shared_ptr<NamespaceRepository> repository_;
  std::shared_ptr<RepositoryRepository> repository_repository_;
};

} // namespace acumio
#endif // AcumioServer_NamespaceService_h
