//============================================================================
// Name        : UserService.cpp
// Author      : Bill Province (bill@acumio.com)
// Version     :
// Copyright   : Copyright 2016
// Description : Service for User operations. At the moment, this does not
//               include any form of Authentication or Authorization. All
//               operations are available to all users, and all entities
//               are accessible and modifiable by all users.
//============================================================================

#include "UserService.h"

namespace acumio {
UserService::UserService(std::unique_ptr<crypto::EncrypterInterface> encrypter,
    std::unique_ptr<crypto::SaltGeneratorInterface> salt_gen) :
    encrypter_(std::move(encrypter)), salt_generator_(std::move(salt_gen)),
    repository_() {}
UserService::~UserService() {}

grpc::Status UserService::GetSelfUser(const std::string& user_name,
                                      model::User* model) {
  FullUser found_user;
  grpc::Status result = repository_.Get(user_name, &found_user);
  if (result.error_code() == grpc::StatusCode::NOT_FOUND) {
    // Underlying library does not know what kind of object it is working
    // with. We re-write the error message here.
    std::string error = "Unable to find user with name: ";
    error += user_name;
    error += ".";
    return grpc::Status(grpc::StatusCode::NOT_FOUND, error);
  }
  else if (!result.ok()) {
    return result;
  }

  model->CopyFrom(found_user.user());
  return grpc::Status::OK;
}

grpc::Status UserService::UserSearch(const server::UserSearchRequest* request,
                                     server::UserSearchResponse* response) {
  // STUB: TODO: IMPLEMENT ME!
  // Hold off on this one a bit. It's rather complicated. Or perhaps,
  // just implement a slimmed down version of it.
  return grpc::Status::OK;
}

grpc::Status UserService::CreateUser(const model::User& user,
                                     const std::string& password) {
  FullUser full_user;
  full_user.mutable_user()->CopyFrom(user);
  // If password is empty, we leave it intentionally empty on full_user.
  if (!password.empty()) {
    std::string salt = (*salt_generator_)();
    std::string encrypted = (*encrypter_)(password, salt);
    full_user.mutable_salt()->assign(salt);
    full_user.mutable_password()->assign(password);
  }
  return repository_.Add(full_user);
}

grpc::Status UserService::RemoveUser(const std::string& user_name) {
  return repository_.Remove(user_name);
}

grpc::Status UserService::UpdateUser(const std::string& user_name,
                                     const model::User& user) {
  FullUser existing_user;
  grpc::Status result = repository_.Get(user_name, &existing_user);
  if (!result.ok()) {
    return result;
  }

  existing_user.mutable_user()->CopyFrom(user);
  return repository_.Update(user_name, existing_user);
}

grpc::Status UserService::UpdatePassword(const std::string& user_name,
                                         const std::string& password) {
  FullUser existing_user;
  grpc::Status result = repository_.Get(user_name, &existing_user);
  if (!result.ok()) {
    return result;
  }
  if (!password.empty()) {
    std::string salt = (*salt_generator_)();
    std::string encrypted = (*encrypter_)(password, salt);
    existing_user.mutable_salt()->assign(salt);
    existing_user.mutable_password()->assign(password);
  } else {
    existing_user.mutable_salt()->clear();
    existing_user.mutable_password()->clear();
  }
  
  return repository_.Update(user_name, existing_user);
}

} // namespace acumio
