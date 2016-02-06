//============================================================================
// Name        : AcumioClient.cpp
// Author      : Bill Province (bill@acumio.com)
// Version     :
// Copyright   : Copyright 2006
// Description : Client-side libraries to connect to AcumioServer.
//============================================================================

#include <iostream>
#include <string>

#include <grpc++/grpc++.h>
#include <grpc++/security/credentials.h>

#include "server.grpc.pb.h"

using namespace std;
using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using acumioserver::ConcatInputRequest;
using acumioserver::ConcatInputResponse;
using acumioserver::Acumio;

namespace acumio {

class ClientConnector {
 public:
  ClientConnector(std::shared_ptr<Channel> channel)
      : stub_(Acumio::NewStub(channel)) {}

  std::string concat(const std::vector<std::string>& inputs,
                     const std::string& separator) {
    ConcatInputRequest request;
    for (auto it = inputs.begin(); it != inputs.end(); ++it) {
      request.add_input(*it);
    }
    request.set_separator(separator);

    ConcatInputResponse response;

    ClientContext context;

    Status status = stub_->ConcatInputs(&context, request, &response);

    if (!status.ok()) {
      cout << "Error: " << status.error_message();
      return "";
    }

    return response.concatenation();
  }

 private:
  std::unique_ptr<Acumio::Stub> stub_;
};

} // end namespace acumio

int main(int argc, char** argv) {
  acumio::ClientConnector client(
      grpc::CreateChannel("localhost:1782",
                          grpc::InsecureChannelCredentials()));

  std::vector<std::string> inputs;
  inputs.push_back("!!!Hello");
  inputs.push_back("World!!!");
  std::string response = client.concat(inputs, " ");
  cout << "Response: " << response << endl;

  return 0;
}
