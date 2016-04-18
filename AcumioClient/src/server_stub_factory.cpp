//============================================================================
// Name        : server_stub_factory.cpp
// Author      : Bill Province (bill@acumio.com)
// Version     :
// Copyright   : Copyright (C) 2016 Acumio
// Description : Factory for Server stubs required by client.
//               There's a "real" one as well as a Mock stub factory.
//============================================================================

#include "server_stub_factory.h"
#include <grpc++/grpc++.h>
#include "server.grpc.pb.h"

namespace acumio {
namespace model {
namespace server {
ServerStubFactoryInterface::ServerStubFactoryInterface() {}
ServerStubFactoryInterface::~ServerStubFactoryInterface() {}
ServerStubFactory::ServerStubFactory() {}
ServerStubFactory::~ServerStubFactory() {}
std::unique_ptr<Server::StubInterface> ServerStubFactory::NewStub(
  const std::shared_ptr<::grpc::ChannelInterface>& channel) {
  return Server::NewStub(channel);
}
} // namespace server
} // namespace model
} // namespace acumio
