//============================================================================
// Name        : test_client.cpp
// Author      : Bill Province (bill@acumio.com)
// Version     :
// Copyright   : Copyright (C) 2016 Acumio
// Description : A test_driver for AcumioClient.cpp using a Mock
//               server stub.
//============================================================================

#include "AcumioClient.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "server_stub_factory.h"
#include "test/mock_server_stub.h"

namespace acumio {
namespace test {
class MockServerStubFactory :
    public acumio::model::server::ServerStubFactoryInterface {
 public:
  MockServerStubFactory() {}
  ~MockServerStubFactory() {}
  std::unique_ptr<Server::StubInterface> NewStub(
      const std::shared_ptr<::grpc::ChannelInterface>& channel) {
    std::unique_ptr<Server::StubInterface> ret_val(
        new acumio::model::server::MockServerStub());
    return ret_val;
  }
};
} // namespace test
} // namespace acumio

TEST(SimpleStubTest, HelloWorldTest) {
  acumio::test::MockServerStubFactory factory;
  auto creds = grpc::InsecureChannelCredentials();
  auto channel = grpc::CreateChannel("localhost:1782", creds);
  std::unique_ptr<Server::StubInterface> mock_stub(factory.NewStub(channel));
  acumio::ClientConnector client(std::move(mock_stub));
  std::vector<std::string> inputs;
  inputs.push_back("!!!Hello");
  inputs.push_back("World!!!");
  std::string response = client.concat(inputs, " ");
  EXPECT_EQ("!!!Hello World!!!", response);
}

int main(int argc, char** argv) {
  ::testing::InitGoogleMock(&argc, argv);
  return RUN_ALL_TESTS();
}
