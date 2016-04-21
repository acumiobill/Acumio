//============================================================================
// Name        : AcumioServer.cpp
// Author      : Bill Province (bill@acumio.com)
// Version     :
// Copyright   : Copyright (C) 2016 Acumio
// Description : Server implementation with grpc.
//               This class has the "main" function as well as being
//               responsible for building the server. The server
//               is constructed of various services that share some repository
//               data. The work for the various services gets delegated
//               to individual components.
//               The division is as follows (currently):
//                    Namespace APIs -- delegated to NamespaceService
//                    User APIs -- delegated to UserService
//                    Dataset APIs -- delegated to DatasetService
//                    Repository APIs -- delegated to RepositoryService.
//
//               Later, we should expect integration with Authentication
//               services as well as some degree of Authorization components.
//               In addition, we will need integration with Audit Logging
//               and APIs for Audit Logging as well as for Lineage APIs.
//
//               Finally, there is a single "Concatenation" service which
//               is really just a kind of "hello world" type of service.
//               This is implemented directly in this class.
//============================================================================

#include <iostream>
#include <fstream>
using namespace std;

#include <grpc++/grpc++.h>

#include <boost/program_options.hpp>

#include "DatasetService.h"
#include "NamespaceService.h"
#include "RepositoryService.h"
#include "UserService.h"
#include "dataset_repository.h"
#include "encrypter.h"
#include "namespace_repository.h"
#include "referential_service.h"
#include "repository_repository.h"
#include "server.grpc.pb.h"

using grpc::ServerContext;
using grpc::ServerCredentials;
using grpc::Status;

namespace po = boost::program_options;

namespace acumio {
namespace model {
namespace server {
class ServerImpl final : public Server::Service {
 public:
  ServerImpl(acumio::DatasetService* dataset_service,
             acumio::NamespaceService* namespace_service,
             acumio::RepositoryService* repository_service,
             acumio::UserService* user_service) :
     dataset_service_(dataset_service),
     namespace_service_(namespace_service),
     repository_service_(repository_service),
     user_service_(user_service) {}

  Status ConcatInputs(ServerContext* context, const ConcatInputRequest* request,
                      ConcatInputResponse* response) override {
    std::string result;
    int last_element = request->input_size() - 1;
    for (int i = 0; i < last_element; i++) {
      result.append(request->input(i)).append(request->separator());
    }
    if (last_element >= 0) {
      result.append(request->input(last_element));
    }

    response->set_concatenation(result);
    return Status::OK;
  }

  Status CreateDataset(ServerContext* context,
                       const CreateDatasetRequest* request,
                       CreateDatasetResponse* response) override {
    return dataset_service_->CreateDataset(request->dataset(),
                                           request->description());
  }

  Status GetDataset(ServerContext* context, const GetDatasetRequest* request,
                    GetDatasetResponse* response) override {
    return dataset_service_->GetDataset(
        request->physical_name(),
        request->included_description_tags(),
        request->included_description_history_tags(),
        response->mutable_dataset(),
        response->mutable_description(),
        response->mutable_description_history());
  }

  Status RemoveDataset(ServerContext* context,
                       const RemoveDatasetRequest* request,
                       RemoveDatasetResponse* response) override {
    return dataset_service_->RemoveDataset(request->name());
  }

  Status SearchDatasets(ServerContext* context,
                        const SearchDatasetsRequest* request,
                        SearchDatasetsResponse* response) override {
    return dataset_service_->SearchDatasets(request, response);
  }

  Status UpdateDataset(ServerContext* context,
                       const UpdateDatasetRequest* request,
                       UpdateDatasetResponse* response) override {
    return dataset_service_->UpdateDataset(request->name(), request->dataset());
  }

  Status UpdateDatasetWithDescription(
      ServerContext* context,
      const UpdateDatasetWithDescriptionRequest* request,
      UpdateDatasetWithDescriptionResponse* response) override {
    return dataset_service_->UpdateDatasetWithDescription(
        request->name(),  request->update(), request->description_update());
  }

  Status UpdateDatasetDescription(
      ServerContext* context,
      const UpdateDatasetDescriptionRequest* request,
      UpdateDatasetDescriptionResponse* response) override {
    return dataset_service_->UpdateDatasetDescription(
        request->name(), request->description_update());
  }

  // Namespace Services
  Status CreateNamespace(ServerContext* context,
                         const CreateNamespaceRequest* request,
                         CreateNamespaceResponse* response) override {
    return namespace_service_->CreateNamespace(request->name_space(),
                                               request->description());
  }

  Status GetNamespace(ServerContext* context,
                      const GetNamespaceRequest* request,
                      GetNamespaceResponse* response) override {
    return namespace_service_->GetNamespace(
        request->full_namespace(), request->include_description(),
        request->include_description_history(), response);
  }

  Status RemoveNamespace(ServerContext* context,
                         const RemoveNamespaceRequest* request,
                         RemoveNamespaceResponse* response) override {
    return namespace_service_->RemoveNamespace(request->namespace_name());
  }

  Status UpdateNamespace(ServerContext* context,
                         const UpdateNamespaceRequest* request,
                         UpdateNamespaceResponse* response) override {
    return namespace_service_->UpdateNamespace(request->namespace_name(),
                                               request->update());
  }

  Status UpdateNamespaceWithDescription(
      ServerContext* context,
      const UpdateNamespaceWithDescriptionRequest* request,
      UpdateNamespaceWithDescriptionResponse* repsonse) override {
    return namespace_service_->UpdateNamespaceWithDescription(
        request->namespace_name(), request->update(),
        request->updated_description(), request->clear_description());
  }

  Status UpsertNamespaceDescription(
      ServerContext* context, const UpsertNamespaceDescriptionRequest* request,
      UpsertNamespaceDescriptionResponse* response) override {
    return namespace_service_->UpsertNamespaceDescription(request->described(),
        request->update(), request->clear_description());
  }

  // Repository Services
  Status CreateRepository(ServerContext* context,
                          const CreateRepositoryRequest* request,
                          CreateRepositoryResponse* response) override {
    return repository_service_->CreateRepository(request->repository(),
        request->description(), request->create_or_associate_namespace(),
        request->namespace_separator());
  }

  Status GetRepository(ServerContext* context,
                       const GetRepositoryRequest* request,
                       GetRepositoryResponse* response) override {
    return repository_service_->GetRepository(request->repository_name(),
        request->include_description(), request->include_description_history(),
        response);
  }

  Status ListRepositories(ServerContext* context,
                        const ListRepositoriesRequest* request,
                        ListRepositoriesResponse* response) override {
    return repository_service_->ListRepositories(request->list_max(),
        request->start_after_name(), request->include_descriptions(),
        response);
  }

  Status RemoveRepository(ServerContext* context,
                          const RemoveRepositoryRequest* request,
                          RemoveRepositoryResponse* response) override {
    return repository_service_->RemoveRepository(request->repository_name(),
        request->force(), request->remove_or_disassociate_namespace());
  }

  Status UpdateRepository(ServerContext* context,
                          const UpdateRepositoryRequest* request,
                          UpdateRepositoryResponse* response) override {
    return repository_service_->UpdateRepository(request->repository_name(),
        request->repository(), request->force());
  }

  Status UpdateRepositoryWithDescription(
      ServerContext* context,
      const UpdateRepositoryWithDescriptionRequest* request,
      UpdateRepositoryWithDescriptionResponse* repsonse) override {
    return repository_service_->UpdateRepositoryWithDescription(
        request->repository_name(), request->update(),
        request->updated_description(), request->clear_description(),
        request->force());
  }

  Status UpsertRepositoryDescription(
      ServerContext* context,
      const UpsertRepositoryDescriptionRequest* request,
      UpsertRepositoryDescriptionResponse* response) override {
    return repository_service_->UpsertRepositoryDescription(
        request->described(), request->update(), request->clear_description());
  }

  // User Services.
  Status CreateUser(ServerContext* context, const CreateUserRequest* request,
                    CreateUserResponse* response) override {
    // TODO: Authenticate User. Standard permissions would have this apply
    // to Admin role.
    return user_service_->CreateUser(request->user(),
                                     request->acumio_password());
  }

  Status GetSelfUser(ServerContext* context, const GetSelfUserRequest* request,
                     GetSelfUserResponse* response) override {
    // TODO: Authenticate user. Later, the GetSelfUserRequest will not
    // even need to contain the username in the request, because it should
    // be inferred by the Authentication proess.
    return user_service_->GetSelfUser(request->user_name(),
                                      response->mutable_user());
  }

  Status RemoveUser(ServerContext* context, const RemoveUserRequest* request,
                    RemoveUserResponse* response) override {
    // TODO: Authenticate User. Standard permissions would have this apply
    // to Admin role.
    return user_service_->RemoveUser(request->user_name());
  }

  Status UpdateUser(ServerContext* context, const UpdateUserRequest* request,
                    UpdateUserResponse* response) override {
    // TODO: Authenticate User. Standard permissions would have this apply
    // to Admin role *unless* the user being updated matched the user being
    // authenticated.
    return user_service_->UpdateUser(request->user_name_to_modify(),
                                     request->updated_user());
  }

  Status UserSearch(ServerContext* context, const UserSearchRequest* request,
                    UserSearchResponse* response) override {
    // TODO: Authenticate User. This should only be callable by Admin role
    // or by special grant to non-Admin user.
    return user_service_->UserSearch(request, response);
  }

 private:
  acumio::DatasetService* dataset_service_;
  acumio::NamespaceService* namespace_service_;
  acumio::RepositoryService* repository_service_;
  acumio::UserService* user_service_;
};
/*
  Bring this back when we are ready to work with ssl communication.
namespace {
void readFileToString(std::string fileName, std::string* results) {
  // Based on timing experiments posted at
  // insanecoding.blogspot.com/2011/11/how-to-read-in-file-in-c.html,
  // the method below is probably the fastest method avaiable to read
  // the entire contents of a file into a string. Basic idea: pre-allocate
  // space for string before reading, and use low-level apis for doing the
  // actual read operations.
  std::ifstream in(fileName, std::ios::in | std::ios::binary);
  // TODO: Add error handling here. Need to select a decent logging mechanism.
  // Any of the methods below could potentially fail.
  in.seekg(0, std::ios::end);
  results->resize(in.tellg());
  in.seekg(0, std::ios::beg);
  in.read(&((*results)[0]), results->size());
  in.close();
}
} // end anonymous namespace.
*/

void RunServer(std::string address) {
  std::unique_ptr<acumio::crypto::EncrypterInterface> encrypter(
    new acumio::crypto::NoOpEncrypter());
  uint64_t salt_seed = 1;
  std::unique_ptr<acumio::crypto::SaltGeneratorInterface> salter(
    new acumio::crypto::DeterministicSaltGenerator(salt_seed));
  UserService user_service(std::move(encrypter), std::move(salter));
  NamespaceRepository namespace_repository;
  RepositoryRepository repository_repository;
  DatasetRepository dataset_repository;
  ReferentialService referential_service(&namespace_repository,
                                         &dataset_repository,
                                         &repository_repository);
  NamespaceService namespace_service(&namespace_repository,
                                     &referential_service);
  RepositoryService repository_service(&repository_repository,
                                       &referential_service);
  DatasetService dataset_service(&dataset_repository, &referential_service);
  ServerImpl service(&dataset_service, &namespace_service,
                     &repository_service, &user_service);
  grpc::ServerBuilder builder;
  /**
   This code assumes ssl connection.
  //int port = 0;
  // Listen on given address without auth mechanism.
  grpc::SslServerCredentialsOptions ssl_opts;
  ssl_opts.pem_root_certs = "";
  std::string key;
  std::string certificate;
  readFileToString(keyFile, &key);
  readFileToString(certKeyFile, &certificate);
  grpc::SslServerCredentialsOptions::PemKeyCertPair pkcp = {key, certificate};
  ssl_opts.pem_key_cert_pairs.push_back(pkcp);
  std::shared_ptr<ServerCredentials> credentials =
      grpc::SslServerCredentials(ssl_opts);
  builder.AddListeningPort(address, credentials);
  **/
  builder.AddListeningPort(address, grpc::InsecureServerCredentials());
  // Register "service" as the instance through which we commmunicate with
  // clients.
  builder.RegisterService(&service);
  // Finally, assemble the server.
  std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << address <<  std::endl;

  // Wait forever for server shutdown. Shutdown operation must be done on
  // separate thread.
  server->Wait();
}
} // end namespace server
} // end namespace model
} // end namespace acumio

int main(int argc, char** argv) {

  po::options_description desc("Allowed/Required options");
  desc.add_options()
      ("help,h", "product help message")
      ("address,a", po::value<string>(),
       "Host + port address of server. Example: mydomain.com:1782. Required. "
       "May also be specified without '-a' flag as positional argument.");
      //("sslKeyFile,k", po::value<string>(), "Name of ssl key file")
      //("certificate", po::value<string>(), "Name of file holding certificate")

  po::positional_options_description p;
  // This specifies that the "address" option is a "positional" option, i.e.,
  // not requiring a flag to specify it. The parameter 1 specifies that we
  // allow exactly 1 address and no more.
  p.add("address", 1);
  po::variables_map var_map;
  po::store(po::command_line_parser(argc, argv).
            options(desc).positional(p).run(),
            var_map);

  if (var_map.count("help") > 0 || var_map.count("address") == 0) {
    cout << desc << "\n";
    cout << "Sample command-line: AcumioServer.exe mydomain.com:1782\n";
    return 1;
  }

  po::notify(var_map);
  std::string address = var_map["address"].as<string>();
/*
  TODO: Clean this up. Commented-out code looks ugly. Saving this until
  we start implementing ssl.
  std::string ssl_key_file = var_map["sslKeyFile"].as<string>();
  std::string certificate = var_map["certificate"].as<string>();
  cout << "keyFile name: " << ssl_key_file << "; certificate file name: "
       << certificate;

  std::string ssl_key_file = "/home/bill/acumio/ssl/private/acumioserver.key";
  std::string certificate = "/home/bill/acumio/ssl/certs/Flying-Ubuntu-Dragon.crt";
*/
  acumio::model::server::RunServer(address);
  return 0;
}
