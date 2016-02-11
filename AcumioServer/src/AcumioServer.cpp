//============================================================================
// Name        : AcumioServer.cpp
// Author      : Bill Province (bill@acumio.com)
// Version     :
// Copyright   : Copyright 2006
// Description : Server skeleton. At the moment, this contains just
//               an implementation of a "string concatenation" service.
//============================================================================

#include <iostream>
using namespace std;

#include <grpc++/grpc++.h>

#include <server.grpc.pb.h>

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using acumio::server::GetUsersRequest;
using acumio::server::GetUsersResponse;
using acumio::server::ConcatInputRequest;
using acumio::server::ConcatInputResponse;

namespace acumio {
class ServerImpl final : public server::Server::Service {
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

  Status GetUsers(ServerContext* context, const GetUsersRequest* request,
                  GetUsersResponse* response) override {
    // STUB: IMPLEMENT ME!
    return Status::OK;
  }

};

}

void RunServer() {
  acumio::ServerImpl service;
  std::string server_address("localhost:1782");
  ServerBuilder builder;
  int port = 0;
  // Listen on given address without auth mechanism.
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  // Register "service" as the instance through which we commmunicate with
  // clients.
  builder.RegisterService(&service);
  // Finally, assemble the server.
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address <<  std::endl;

  // Wait forever for server shutdown. Shutdown operation must be done on
  // separate thread.
  server->Wait();
}

int main(int argc, char** argv) {
	RunServer();
	return 0;
}
