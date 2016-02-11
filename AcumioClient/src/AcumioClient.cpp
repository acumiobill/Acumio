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
#include "AcumioClient.h"

using namespace std;
using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using acumio::server::ConcatInputRequest;
using acumio::server::ConcatInputResponse;
using acumio::server::Server;

namespace acumio {

std::string ClientConnector::concat(const std::vector<std::string>& inputs,
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
} // end namespace acumio
