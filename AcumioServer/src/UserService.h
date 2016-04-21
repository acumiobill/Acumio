#ifndef AcumioServer_UserService_h
#define AcumioServer_UserService_h
//============================================================================
// Name        : UserService.h
// Author      : Bill Province (bill@acumio.com)
// Version     :
// Copyright   : Copyright (C) 2016 Acumio
// Description : Service for User operations. At the moment, this does not
//               include any form of Authentication or Authorization. All
//               operations are available to all users, and all entities
//               are accessible and modifiable by all users.
//============================================================================

#include <grpc++/grpc++.h>
#include "server.pb.h"
#include "user.pb.h"

#include "encrypter.h"
#include "user_repository.h"

namespace acumio {

class UserService {
 public:
  UserService(std::unique_ptr<crypto::EncrypterInterface> encrypter,
              std::unique_ptr<crypto::SaltGeneratorInterface> salt_generator);
  ~UserService();
  grpc::Status GetSelfUser(const std::string& user_name,
                           model::User* user);
  grpc::Status UserSearch(const model::server::UserSearchRequest* request,
                          model::server::UserSearchResponse* response);
  grpc::Status CreateUser(const model::User& user,
                          const std::string& password);
  grpc::Status RemoveUser(const std::string& user_name);
  grpc::Status UpdateUser(const std::string& user_name,
                          const model::User& user);
  grpc::Status UpdatePassword(const std::string& user_name,
                              const std::string& password);

 private:
  // The encrypter_ will be invoked as:
  // std::string salt = salt_generator_();
  // std::string encrypted = encrypter_(pw, salt);
  // where pw is a password provided by the API, and salt is a randomly
  // generated string.
  // When we store passwords, we of course, do not store the clear password,
  // rather, we store the encrypted password and the salt.
  std::unique_ptr<crypto::EncrypterInterface> encrypter_;

  std::unique_ptr<crypto::SaltGeneratorInterface> salt_generator_;

  // Underlying User storage.
  UserRepository repository_;
};
} // namespace acumio

#endif // AcumioServer_UserService_h
