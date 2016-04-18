#ifndef AcumioClient_server_stub_factory_h
#define AcumioClient_server_stub_factory_h

#include <grpc++/grpc++.h>
#include "server.grpc.pb.h"

namespace acumio {
namespace model {
namespace server {

class ServerStubFactoryInterface {
 public:
  ServerStubFactoryInterface();
  virtual ~ServerStubFactoryInterface();
  virtual std::unique_ptr<Server::StubInterface> NewStub(
      const std::shared_ptr<::grpc::ChannelInterface>& channel) = 0;
};

class ServerStubFactory : public ServerStubFactoryInterface {
 public:
  ServerStubFactory();
  ~ServerStubFactory();
  std::unique_ptr<Server::StubInterface> NewStub(
      const std::shared_ptr<::grpc::ChannelInterface>& channel);
};

} // namespace server
} // namespace model
} // namespace acumio
#endif // AcumioClient_server_stub_factory_h
