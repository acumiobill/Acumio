//============================================================================
// Name        : AcumioServer.cpp
// Author      : Bill Province (bill@acumio.com)
// Version     :
// Copyright   : Copyright 2016
// Description : Server skeleton. At the moment, this contains just
//               an implementation of a "string concatenation" service
//               and hooks for connecting to a User Service, which responds
//               to the user-based APIs.
//============================================================================

#include <iostream>
#include <fstream>
using namespace std;

#include <grpc++/grpc++.h>

#include <server.grpc.pb.h>
#include <boost/program_options.hpp>

#include "encrypter.h"
#include "UserService.h"

using grpc::ServerContext;
using grpc::ServerCredentials;
using grpc::Status;

//namespace po  = boost::program_options;

namespace acumio {
namespace server {
class ServerImpl final : public Server::Service {
 public:
  ServerImpl(std::unique_ptr<acumio::UserService> user_service) :
     user_service_(std::move(user_service)) {}

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

  Status GetSelfUser(ServerContext* context, const GetSelfUserRequest* request,
                     GetSelfUserResponse* response) override {
    // TODO: Authenticate user. Later, the GetSelfUserRequest will not
    // even need to contain the username in the request, because it should
    // be inferred by the Authentication proess.
    return user_service_->GetSelfUser(request->username(),
                                      response->mutable_user());
  }

  Status UserSearch(ServerContext* context, const UserSearchRequest* request,
                    UserSearchResponse* response) override {
    // TODO: Authenticate User. This should only be callable by Admin role
    // or by special grant to non-Admin user.
    return user_service_->UserSearch(request, response);
  }

  Status CreateUser(ServerContext* context, const CreateUserRequest* request,
                    CreateUserResponse* response) override {
    // TODO: Authenticate User. Standard permissions would have this apply
    // to Admin role.
    return user_service_->CreateUser(request->user(),
                                     request->acumiopassword());
  }

  Status RemoveUser(ServerContext* context, const RemoveUserRequest* request,
                    RemoveUserResponse* response) override {
    // TODO: Authenticate User. Standard permissions would have this apply
    // to Admin role.
    return user_service_->RemoveUser(request->username());
  }

  Status UpdateUser(ServerContext* context, const UpdateUserRequest* request,
                    UpdateUserResponse* response) override {
    // TODO: Authenticate User. Standard permissions would have this apply
    // to Admin role *unless* the user being updated matched the user being
    // authenticated.
    return user_service_->UpdateUser(request->usernametomodify(),
                                     request->updateduser());
  }

 private:
  std::unique_ptr<acumio::UserService> user_service_;
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

void RunServer(std::string keyFile, std::string certKeyFile) {
  std::unique_ptr<acumio::crypto::EncrypterInterface> encrypter(
    new acumio::crypto::NoOpEncrypter());
  uint64_t salt_seed = 1;
  std::unique_ptr<acumio::crypto::SaltGeneratorInterface> salter(
    new acumio::crypto::DeterministicSaltGenerator(salt_seed));
  std::unique_ptr<acumio::UserService> user_service(
      new UserService(std::move(encrypter), std::move(salter)));
  ServerImpl service(std::move(user_service));
  std::string server_address("Flying-Ubuntu-Dragon:1782");
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
  builder.AddListeningPort(server_address, credentials);
  **/
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  // Register "service" as the instance through which we commmunicate with
  // clients.
  builder.RegisterService(&service);
  // Finally, assemble the server.
  std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address <<  std::endl;

  // Wait forever for server shutdown. Shutdown operation must be done on
  // separate thread.
  server->Wait();
}
} // end namespace server
} // end namespace acumio

int main(int argc, char** argv) {
/*
  po::options_description desc("Allowed options");
  desc.add_options()
      ("help", "product help message")
      ("sslKeyFile", po::value<string>(), "Name of ssl key file")
      ("certificate", po::value<string>(), "Name of file holding certificate")
  ;

  po::variables_map var_map;
  po::store(po::parse_command_line(argc, argv, desc), var_map);
  po::notify(var_map);

  if (var_map.count("help") > 0) {
    cout << desc << "\n";
    return 1;
  }

  std::string ssl_key_file = var_map["sslKeyFile"].as<string>();
  std::string certificate = var_map["certificate"].as<string>();
  cout << "keyFile name: " << ssl_key_file << "; certificate file name: "
       << certificate;
*/
  std::string ssl_key_file = "/home/bill/acumio/ssl/private/acumioserver.key";
  std::string certificate = "/home/bill/acumio/ssl/certs/Flying-Ubuntu-Dragon.crt";
  acumio::server::RunServer(ssl_key_file, certificate);
  return 0;
}
