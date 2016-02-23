//============================================================================
// Name        : AcumioCommandLine.cpp
// Author      : Bill Province (bill@acumio.com)
// Version     :
// Copyright   : Copyright 2006
// Description : Command-Line client to Acumio Server.
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

int main(int argc, char** argv) {
  //auto creds = grpc::SslCredentials(grpc::SslCredentialsOptions());
  auto creds = grpc::InsecureChannelCredentials();
  auto channel = grpc::CreateChannel("Flying-Ubuntu-Dragon:1782", creds);
  acumio::ClientConnector client(channel);

  std::vector<std::string> inputs;
  inputs.push_back("!!!Hello");
  inputs.push_back("World!!!");
  std::string response = client.concat(inputs, " ");
  cout << "Response: " << response << endl;

  return 0;
}
