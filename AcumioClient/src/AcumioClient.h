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
using acumio::server::Server;

namespace acumio {

class ClientConnector {
 public:
  ClientConnector(std::shared_ptr<Channel> channel)
      : stub_(Server::NewStub(channel)) {}

  std::string concat(const std::vector<std::string>& inputs,
                     const std::string& separator);

 private:
  std::unique_ptr<Server::Stub> stub_;
};

} // end namespace acumio
