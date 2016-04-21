#ifndef AcumioServer_user_repository_h
#define AcumioServer_user_repository_h
//============================================================================
// Name        : user_repository.h
// Author      : Bill Province (bill@acumio.com)
// Version     :
// Copyright   : Copyright (C) 2016 Acumio
// Description : Provides multi-threaded access to an in-memory
//               repository of User information.
//============================================================================

#include "mem_repository.h"
#include "user.pb.h"
#include <grpc++/support/status.h>

namespace acumio {

class FullUser {
 public:
  FullUser() : user_(), password_(), salt_() {}
  FullUser(const model::User& user,
           const std::string& encrypted_password,
           const std::string& salt) : user_(user),
              password_(encrypted_password), salt_(salt) {}
  ~FullUser();

  inline const model::User& user() const {return user_;}
  inline const std::string& password() const {return password_;}
  inline const std::string& salt() const {return salt_;}
  inline model::User* mutable_user() { return &user_; }
  inline std::string* mutable_password() { return &password_; }
  inline std::string* mutable_salt() { return &salt_; }

 private:
  model::User user_;
  std::string password_;
  std::string salt_;
};


class UserRepository {
 public:
  typedef mem_repository::MemRepository<FullUser> _UserRepository;
  typedef _UserRepository::PrimaryIterator PrimaryIterator;
  typedef _UserRepository::SecondaryIterator SecondaryIterator;

  UserRepository();
  ~UserRepository();

  inline grpc::Status Add(const FullUser& user) {
    return repository_->Add(user);
  }
  inline grpc::Status Remove(const std::string& name) {
    std::unique_ptr<Comparable> key(new StringComparable(name));
    return repository_->Remove(key);
  }
  inline grpc::Status Update(const std::string& name,
                             const FullUser& user) {
    std::unique_ptr<Comparable> key(new StringComparable(name));
    return  repository_->Update(key, user);
  }

  inline grpc::Status Get(const std::string& name, FullUser* elt) {
    std::unique_ptr<Comparable> key(new StringComparable(name));
    // TODO: Translate NOT_FOUND results to present better error message.
    return repository_->Get(key, elt);
  }

  inline int32_t size() const { return repository_->size(); }

  PrimaryIterator LowerBoundByName(const std::string& name) {
    std::unique_ptr<Comparable> key(new StringComparable(name));
    return repository_->LowerBound(key);
  }
/*
  Re-enable this when we start want to support search more effectively.

  SecondaryIterator LowerBoundByEmail(const std::string& email) const {
    std::unique_ptr<Comparable> key(new StringComparable(email));
    return repository_->LowerBoundByIndex(string_key, 0);
  }
*/
 private:
  std::unique_ptr<_UserRepository> repository_;
};

} // namespace acumio

#endif // AcumioServer_user_repository_h
