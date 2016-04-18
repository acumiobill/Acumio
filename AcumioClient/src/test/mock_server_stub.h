#ifndef AcumioClient_mock_server_stub_h
#define AcumioClient_mock_server_stub_h
//============================================================================
// Name        : mock_server_stub.h
// Author      : Bill Province (bill@acumio.com)
// Version     :
// Copyright   : Copyright (C) 2016 Acumio
// Description : Mock class for providing a Stub service to the AcumioServer.
//============================================================================
#include <gmock/gmock.h>
#include <grpc++/grpc++.h>
#include "server.grpc.pb.h"
#include "server.pb.h"

namespace acumio {
namespace model {
namespace server {

class MockServerStub : public Server::StubInterface {
 public:
  MockServerStub();
  ~MockServerStub();

  ::grpc::Status ConcatInputs(::grpc::ClientContext* context,
                              const ConcatInputRequest& request,
                              ConcatInputResponse* response) {
    std::string result;
    int last_element = request.input_size() - 1;
    for (int i = 0; i < last_element; i++) {
      result.append(request.input(i)).append(request.separator());
    }
    if (last_element >= 0) {
      result.append(request.input(last_element));
    }

    response->set_concatenation(result);
    return grpc::Status::OK;
  }

  MOCK_METHOD3(CreateDataset,
               ::grpc::Status(::grpc::ClientContext* context,
                              const CreateDatasetRequest& request,
                              CreateDatasetResponse* response));
  MOCK_METHOD3(GetDataset,
               ::grpc::Status(::grpc::ClientContext* context,
                              const GetDatasetRequest& request,
                              GetDatasetResponse* response));
  MOCK_METHOD3(RemoveDataset,
               ::grpc::Status(::grpc::ClientContext* context,
                              const RemoveDatasetRequest& request,
                              RemoveDatasetResponse* response));

  MOCK_METHOD3(SearchDatasets,
               ::grpc::Status(::grpc::ClientContext* context,
                              const SearchDatasetsRequest& request,
                              SearchDatasetsResponse* response));

  MOCK_METHOD3(UpdateDataset,
               ::grpc::Status(::grpc::ClientContext* context,
                              const UpdateDatasetRequest& request,
                              UpdateDatasetResponse* response));

  MOCK_METHOD3(UpdateDatasetWithDescription,
               ::grpc::Status(::grpc::ClientContext* context,
                   const UpdateDatasetWithDescriptionRequest& request,
                   UpdateDatasetWithDescriptionResponse* response));

  MOCK_METHOD3(UpdateDatasetDescription,
               ::grpc::Status(::grpc::ClientContext* context,
                              const UpdateDatasetDescriptionRequest& request,
                              UpdateDatasetDescriptionResponse* response));

  MOCK_METHOD3(CreateNamespace,
               ::grpc::Status(::grpc::ClientContext* context,
                              const CreateNamespaceRequest& request,
                              CreateNamespaceResponse* response));

  MOCK_METHOD3(GetNamespace,
               ::grpc::Status(::grpc::ClientContext* context,
                              const GetNamespaceRequest& request,
                              GetNamespaceResponse* response));

  MOCK_METHOD3(RemoveNamespace,
               ::grpc::Status(::grpc::ClientContext* context,
                              const RemoveNamespaceRequest& request,
                              RemoveNamespaceResponse* response));

  MOCK_METHOD3(UpdateNamespace,
               ::grpc::Status(::grpc::ClientContext* context,
                              const UpdateNamespaceRequest& request,
                              UpdateNamespaceResponse* response));

  MOCK_METHOD3(UpdateNamespaceWithDescription,
               ::grpc::Status(::grpc::ClientContext* context,
                   const UpdateNamespaceWithDescriptionRequest& request,
                   UpdateNamespaceWithDescriptionResponse* response));

  MOCK_METHOD3(UpsertNamespaceDescription,
               ::grpc::Status(::grpc::ClientContext* context,
                              const UpsertNamespaceDescriptionRequest& request,
                              UpsertNamespaceDescriptionResponse* response));

  MOCK_METHOD3(CreateRepository,
               ::grpc::Status(::grpc::ClientContext* context,
                              const CreateRepositoryRequest& request,
                              CreateRepositoryResponse* response));

  MOCK_METHOD3(GetRepository,
               ::grpc::Status(::grpc::ClientContext* context,
                              const GetRepositoryRequest& request,
                              GetRepositoryResponse* response));

  MOCK_METHOD3(ListRepositories,
               ::grpc::Status(::grpc::ClientContext* context,
                              const ListRepositoriesRequest& request,
                              ListRepositoriesResponse* response));

  MOCK_METHOD3(RemoveRepository,
               ::grpc::Status(::grpc::ClientContext* context,
                              const RemoveRepositoryRequest& request,
                              RemoveRepositoryResponse* response));

  MOCK_METHOD3(UpdateRepository,
               ::grpc::Status(::grpc::ClientContext* context,
                              const UpdateRepositoryRequest& request,
                              UpdateRepositoryResponse* response));

  MOCK_METHOD3(UpdateRepositoryWithDescription,
               ::grpc::Status(::grpc::ClientContext* context,
                   const UpdateRepositoryWithDescriptionRequest& request,
                   UpdateRepositoryWithDescriptionResponse* response));

  MOCK_METHOD3(UpsertRepositoryDescription,
               ::grpc::Status(::grpc::ClientContext* context,
                   const UpsertRepositoryDescriptionRequest& request,
                   UpsertRepositoryDescriptionResponse* response));

  MOCK_METHOD3(CreateUser,
               ::grpc::Status(::grpc::ClientContext* context,
                   const CreateUserRequest& request,
                   CreateUserResponse* response));

  MOCK_METHOD3(GetSelfUser,
               ::grpc::Status(::grpc::ClientContext* context,
                   const GetSelfUserRequest& request,
                   GetSelfUserResponse* response));

  MOCK_METHOD3(RemoveUser,
               ::grpc::Status(::grpc::ClientContext* context,
                   const RemoveUserRequest& request,
                   RemoveUserResponse* response));

  MOCK_METHOD3(UpdateUser,
               ::grpc::Status(::grpc::ClientContext* context,
                   const UpdateUserRequest& request,
                   UpdateUserResponse* response));

  MOCK_METHOD3(UserSearch,
               ::grpc::Status(::grpc::ClientContext* context,
                   const UserSearchRequest& request,
                   UserSearchResponse* response));
 private:
  ::grpc::ClientAsyncResponseReaderInterface<ConcatInputResponse>*
  AsyncConcatInputsRaw(::grpc::ClientContext* context,
                       const ConcatInputRequest& request,
                       ::grpc::CompletionQueue* cq) GRPC_OVERRIDE {
    return nullptr;
  }

  ::grpc::ClientAsyncResponseReaderInterface<CreateDatasetResponse>*
  AsyncCreateDatasetRaw(::grpc::ClientContext* context,
                        const CreateDatasetRequest& request,
                        ::grpc::CompletionQueue* cq) GRPC_OVERRIDE {
    return nullptr;
  }

  ::grpc::ClientAsyncResponseReaderInterface<GetDatasetResponse>*
  AsyncGetDatasetRaw(::grpc::ClientContext* context,
                     const GetDatasetRequest& request,
                     ::grpc::CompletionQueue* cq) GRPC_OVERRIDE {
    return nullptr;
  }

  ::grpc::ClientAsyncResponseReaderInterface<RemoveDatasetResponse>*
  AsyncRemoveDatasetRaw(::grpc::ClientContext* context,
                        const RemoveDatasetRequest& request,
                        ::grpc::CompletionQueue* cq) GRPC_OVERRIDE {
    return nullptr;
  }

  ::grpc::ClientAsyncResponseReaderInterface<SearchDatasetsResponse>*
  AsyncSearchDatasetsRaw(::grpc::ClientContext* context,
                         const SearchDatasetsRequest& request,
                         ::grpc::CompletionQueue* cq) GRPC_OVERRIDE {
    return nullptr;
  }

  ::grpc::ClientAsyncResponseReaderInterface<UpdateDatasetResponse>*
  AsyncUpdateDatasetRaw(::grpc::ClientContext* context,
                        const UpdateDatasetRequest& request,
                        ::grpc::CompletionQueue* cq) GRPC_OVERRIDE {
    return nullptr;
  }

  ::grpc::ClientAsyncResponseReaderInterface<
      UpdateDatasetWithDescriptionResponse>*
  AsyncUpdateDatasetWithDescriptionRaw(
      ::grpc::ClientContext* context,
      const UpdateDatasetWithDescriptionRequest& request,
      ::grpc::CompletionQueue* cq) GRPC_OVERRIDE {
    return nullptr;
  }

  ::grpc::ClientAsyncResponseReaderInterface<UpdateDatasetDescriptionResponse>*
  AsyncUpdateDatasetDescriptionRaw(
      ::grpc::ClientContext* context,
      const UpdateDatasetDescriptionRequest& request,
      ::grpc::CompletionQueue* cq) GRPC_OVERRIDE {
    return nullptr;
  }

  ::grpc::ClientAsyncResponseReaderInterface<CreateNamespaceResponse>*
  AsyncCreateNamespaceRaw(::grpc::ClientContext* context,
                          const CreateNamespaceRequest& request,
                          ::grpc::CompletionQueue* cq) GRPC_OVERRIDE {
    return nullptr;
  }

  ::grpc::ClientAsyncResponseReaderInterface<GetNamespaceResponse>*
  AsyncGetNamespaceRaw(::grpc::ClientContext* context,
                       const GetNamespaceRequest& request,
                       ::grpc::CompletionQueue* cq) GRPC_OVERRIDE {
    return nullptr;
  }

  ::grpc::ClientAsyncResponseReaderInterface<RemoveNamespaceResponse>*
  AsyncRemoveNamespaceRaw(::grpc::ClientContext* context,
                          const RemoveNamespaceRequest& request,
                          ::grpc::CompletionQueue* cq) GRPC_OVERRIDE {
    return nullptr;
  }

  ::grpc::ClientAsyncResponseReaderInterface<UpdateNamespaceResponse>*
  AsyncUpdateNamespaceRaw(::grpc::ClientContext* context,
                          const UpdateNamespaceRequest& request,
                          ::grpc::CompletionQueue* cq) GRPC_OVERRIDE {
    return nullptr;
  }

  ::grpc::ClientAsyncResponseReaderInterface<
      UpdateNamespaceWithDescriptionResponse>*
  AsyncUpdateNamespaceWithDescriptionRaw(
      ::grpc::ClientContext* context,
      const UpdateNamespaceWithDescriptionRequest& request,
      ::grpc::CompletionQueue* cq) GRPC_OVERRIDE {
    return nullptr;
  }

  ::grpc::ClientAsyncResponseReaderInterface<
      UpsertNamespaceDescriptionResponse>*
  AsyncUpsertNamespaceDescriptionRaw(
      ::grpc::ClientContext* context,
      const UpsertNamespaceDescriptionRequest& request,
      ::grpc::CompletionQueue* cq) GRPC_OVERRIDE {
    return nullptr;
  }

  ::grpc::ClientAsyncResponseReaderInterface<CreateRepositoryResponse>*
  AsyncCreateRepositoryRaw(::grpc::ClientContext* context,
                           const CreateRepositoryRequest& request,
                           ::grpc::CompletionQueue* cq) GRPC_OVERRIDE {
    return nullptr;
  }

  ::grpc::ClientAsyncResponseReaderInterface<GetRepositoryResponse>*
  AsyncGetRepositoryRaw(::grpc::ClientContext* context,
                        const GetRepositoryRequest& request,
                        ::grpc::CompletionQueue* cq) GRPC_OVERRIDE {
    return nullptr;
  }

  ::grpc::ClientAsyncResponseReaderInterface<ListRepositoriesResponse>*
  AsyncListRepositoriesRaw(::grpc::ClientContext* context,
                           const ListRepositoriesRequest& request,
                           ::grpc::CompletionQueue* cq) GRPC_OVERRIDE {
    return nullptr;
  }

  ::grpc::ClientAsyncResponseReaderInterface<RemoveRepositoryResponse>* AsyncRemoveRepositoryRaw(::grpc::ClientContext* context,
                 const RemoveRepositoryRequest& request,
                 ::grpc::CompletionQueue* cq) GRPC_OVERRIDE {
    return nullptr;
  }

  ::grpc::ClientAsyncResponseReaderInterface<UpdateRepositoryResponse>*
  AsyncUpdateRepositoryRaw(::grpc::ClientContext* context,
                           const UpdateRepositoryRequest& request,
                           ::grpc::CompletionQueue* cq) GRPC_OVERRIDE {
    return nullptr;
  }

  ::grpc::ClientAsyncResponseReaderInterface<
      UpdateRepositoryWithDescriptionResponse>*
  AsyncUpdateRepositoryWithDescriptionRaw(
      ::grpc::ClientContext* context,
      const UpdateRepositoryWithDescriptionRequest& request,
      ::grpc::CompletionQueue* cq) GRPC_OVERRIDE {
    return nullptr;
  }

  ::grpc::ClientAsyncResponseReaderInterface<
      UpsertRepositoryDescriptionResponse>*
  AsyncUpsertRepositoryDescriptionRaw(
      ::grpc::ClientContext* context,
      const UpsertRepositoryDescriptionRequest& request,
      ::grpc::CompletionQueue* cq) GRPC_OVERRIDE {
    return nullptr;
  }

  ::grpc::ClientAsyncResponseReaderInterface<CreateUserResponse>*
  AsyncCreateUserRaw(::grpc::ClientContext* context,
                     const CreateUserRequest& request,
                     ::grpc::CompletionQueue* cq) GRPC_OVERRIDE {
    return nullptr;
  }

  ::grpc::ClientAsyncResponseReaderInterface<GetSelfUserResponse>*
  AsyncGetSelfUserRaw(::grpc::ClientContext* context,
                      const GetSelfUserRequest& request,
                      ::grpc::CompletionQueue* cq) GRPC_OVERRIDE {
    return nullptr;
  }

  ::grpc::ClientAsyncResponseReaderInterface<RemoveUserResponse>*
  AsyncRemoveUserRaw(::grpc::ClientContext* context,
                     const RemoveUserRequest& request,
                     ::grpc::CompletionQueue* cq) GRPC_OVERRIDE {
    return nullptr;
  }

  ::grpc::ClientAsyncResponseReaderInterface<UpdateUserResponse>*
  AsyncUpdateUserRaw(::grpc::ClientContext* context,
                     const UpdateUserRequest& request,
                     ::grpc::CompletionQueue* cq) GRPC_OVERRIDE {
    return nullptr;
  }

  ::grpc::ClientAsyncResponseReaderInterface<UserSearchResponse>*
  AsyncUserSearchRaw(::grpc::ClientContext* context,
                     const UserSearchRequest& request,
                     ::grpc::CompletionQueue* cq) GRPC_OVERRIDE {
    return nullptr;
  }
};

} // namespace acumio
} // namespace model
} // namespace server

#endif // AcumioClient_mock_server_stub_h
