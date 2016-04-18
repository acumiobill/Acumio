//============================================================================
// Name        : AcumioCommandLine.cpp
// Author      : Bill Province (bill@acumio.com)
// Version     :
// Copyright   : Copyright (C) 2016 Acumio
// Description : Command-Line client to Acumio Server.
//============================================================================

#include <boost/program_options.hpp>
#include <chrono>
#include <grpc++/grpc++.h>
#include <grpc++/security/credentials.h>
#include <iostream>
#include <string>

#include "server.grpc.pb.h"
#include "AcumioClient.h"
#include "server_stub_factory.h"

using namespace std;
using grpc::Channel;
using grpc::ClientContext;

namespace po = boost::program_options;

int main(int argc, char** argv) {
  po::options_description desc("Allowed/Required options");
  desc.add_options()
      ("help,h", "This help message")
      ("address,a", po::value<string>(),
       "Host + port address of server. Example: serverdomain.com:1782. "
       "Required. May also be specified without -a as positional argument.");

  po::positional_options_description p;
  p.add("address", 1);
  po::variables_map var_map;
  po::store(po::command_line_parser(argc, argv).
            options(desc).positional(p).run(),
            var_map);

  if (var_map.count("help") > 0 || var_map.count("address") == 0) {
    cout << desc << "\n";
    cout << "Sample command-line: AcumioCommandLine.exe serverdomain.com:1782";
    cout << "\n";
    return 1;
  }

  po::notify(var_map);
  std::string address = var_map["address"].as<string>();

  //auto creds = grpc::SslCredentials(grpc::SslCredentialsOptions());
  auto creds = grpc::InsecureChannelCredentials();
  auto channel = grpc::CreateChannel(address, creds);
  acumio::model::server::ServerStubFactory stub_factory;
  acumio::ClientConnector client(stub_factory.NewStub(channel));

  std::vector<std::string> inputs;
  inputs.push_back("!!!Hello");
  inputs.push_back("World!!!");
  std::chrono::time_point<std::chrono::system_clock> start, end;
  start = std::chrono::system_clock::now();
  std::string response = client.concat(inputs, " ");
  end = std::chrono::system_clock::now();
  auto micros =
      std::chrono::duration_cast<std::chrono::microseconds>(end - start);
  cout << "Response: " << response << "; Calculated in " << micros.count()
       << " microseconds." << endl;

  return 0;
}
