#ifndef AcumioServer_NamespaceService_h
#define AcumioServer_NamespaceService_h
//============================================================================
// Name        : DatasetService.h
// Author      : Bill Province (bill@acumio.com)
// Version     :
// Copyright   : Copyright 2016
// Description : Service for Namespace operations.
//============================================================================

#include <grpc++/grpc++.h>
#include <server.pb.h>
#include <names.pb.h>
#include "namespace_repository.h"

namespace acumio {

class NamespaceService {
 public:
  NamespaceService(std::shared_ptr<NamespaceRepository> repository);
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
      const model::Description updated_description,
      bool clear_description);

  grpc::Status UpsertNamespaceDescription(const std::string& described,
      const model::Description& update, bool clear_description);

 private:
  // Used internally when we have a GetNamespace with the request to just
  // get the Namespace without the Descriptions.
  grpc::Status GetJustNamespace(const std::string& full_namespace,
                                model::server::GetNamespaceResponse* response);

  // This is a shared ptr instead of a unique ptr because other services may
  // also need access to the Namespace repository.
  std::shared_ptr<NamespaceRepository> repository_;
};

} // namespace acumio
#endif // AcumioServer_NamespaceService_h
