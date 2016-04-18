#ifndef AcumioClient_AcumioClient_h
#define AcumioClient_AcumioClient_h
//============================================================================
// Name        : AcumioClient.h
// Author      : Bill Province (bill@acumio.com)
// Version     :
// Copyright   : Copyright (C) 2016 Acumio
// Description : Client-side libraries to connect to AcumioServer.
//============================================================================

#include <iostream>
#include <string>

#include <grpc++/grpc++.h>
#include <grpc++/security/credentials.h>

#include "server.grpc.pb.h"

using namespace std;
using grpc::Channel;
using acumio::model::server::Server;

namespace acumio {

class ClientConnector {
 public:
  ClientConnector(std::unique_ptr<model::server::Server::StubInterface> stub)
      : stub_(std::move(stub)) {}

  std::string concat(const std::vector<std::string>& inputs,
                     const std::string& separator);

 private:
  std::unique_ptr<model::server::Server::StubInterface> stub_;
};

} // end namespace acumio

#endif // AcumioClient_AcumioClient_h
