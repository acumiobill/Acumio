#ifndef AcumioServer_user_repository_h
#define AcumioServer_user_repository_h
//============================================================================
// Name        : user_repository.h
// Author      : Bill Province (bill@acumio.com)
// Version     :
// Copyright   : Copyright 2016
// Description : Provides multi-threaded access to an in-memory
//               repository of User information.
//============================================================================

#include "mem_repository.h"
#include <user.pb.h>
#include <grpc++/support/status.h>

namespace acumio {

class FullUser {
 public:
  FullUser() : user_(), password_(), salt_() {}
  FullUser(const acumio::model::User& user,
           const std::string& encrypted_password,
           const std::string& salt) : user_(user),
              password_(encrypted_password), salt_(salt) {}
  ~FullUser();

  inline const acumio::model::User& user() const {return user_;}
  inline const std::string password() const {return password_;}
  inline const std::string salt() const {return salt_;}
  inline acumio::model::User* mutable_user() { return &user_; }
  inline std::string* mutable_password() { return &password_; }
  inline std::string* mutable_salt() { return &salt_; }

 private:
  acumio::model::User user_;
  std::string password_;
  std::string salt_;
};

typedef acumio::mem_repository::KeyExtractorInterface<FullUser> _UserExtractor;
typedef acumio::mem_repository::MemRepository<FullUser> _UserRepository;

class NameExtractor : public _UserExtractor {
 public:
  NameExtractor() : _UserExtractor() {}
  ~NameExtractor();
  inline std::unique_ptr<Comparable> GetKey(const FullUser& element) const {
    std::unique_ptr<Comparable> key(
        new StringComparable(element.user().name()));
    return key;
  }
};

class UserRepository {
 public:
  typedef mem_repository::MemRepository<FullUser>::PrimaryIterator
      PrimaryIterator;
  typedef mem_repository::MemRepository<FullUser>::SecondaryIterator
      SecondaryIterator;

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

  PrimaryIterator LowerBoundByName(const std::string& name) {
    std::unique_ptr<Comparable> key(new StringComparable(name));
    return repository_->LowerBound(key);
  }
/*
  SecondaryIterator LowerBoundByIndex(const std::unique_ptr<Comparable>& key,
                                      int index_number) const {
    std::unique_ptr<Comparable> string_key(new StringComparable(key));
    return repository_->LowerBoundByIndex(string_key, index_number);
  }
*/
 private:
  std::unique_ptr<_UserRepository> repository_;
};

} // namespace acumio

#endif // AcumioServer_user_repository_h
